/*
 * rdap-cli - A lightweight RDAP-only command-line client
 *
 * Copyright (c) 2026 OsbornePro
 * Author: OsbornePro
 *
 * SPDX-License-Identifier: MIT
 *
 * Project: https://github.com/OsbornePro/rdap-cli
 */
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <curl/curl.h>
#include <errno.h>
#include <json-c/json.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define VERSION "0.3.1"

#define IANA_DNS  "https://data.iana.org/rdap/dns.json"
#define IANA_IPV4 "https://data.iana.org/rdap/ipv4.json"
#define IANA_IPV6 "https://data.iana.org/rdap/ipv6.json"
#define IANA_ASN  "https://data.iana.org/rdap/asn.json"

struct buffer {
    char *data;
    size_t len;
};

enum object_type {
    OBJ_DOMAIN,
    OBJ_IPV4,
    OBJ_IPV6,
    OBJ_ASN,
    OBJ_UNKNOWN
};

static void usage(FILE *out, const char *prog)
{
    fprintf(out,
        "Usage: %s [options] QUERY\n"
        "\n"
        "RDAP-only registration lookup client.\n"
        "\n"
        "Examples:\n"
        "  %s example.com\n"
        "  %s 1.1.1.1\n"
        "  %s 2606:4700:4700::1111\n"
        "  %s AS15169\n"
        "  %s --json example.com\n"
        "  %s --notices example.com\n"
        "  %s --links example.com\n"
        "  %s --follow example.com\n"
        "  %s --json --follow example.com\n"
        "  %s --server example.com\n"
        "\n"
        "Options:\n"
        "  -j, --json       Print complete RDAP JSON; with --follow, wrap both responses\n"
        "  -n, --notices    Include RDAP notices in human-readable output\n"
        "  -l, --links      Include RDAP links in human-readable output\n"
        "  -f, --follow     Follow a related registrar RDAP link when available\n"
        "  -s, --server     Print the discovered authoritative RDAP base URL\n"
        "  -v, --verbose    Show bootstrap discovery details on stderr\n"
        "  -V, --version    Print version and exit\n"
        "  -h, --help       Show this help\n",
        prog, prog, prog, prog, prog, prog, prog, prog, prog, prog, prog);
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t n = size * nmemb;
    struct buffer *buf = userdata;
    char *tmp = realloc(buf->data, buf->len + n + 1);

    if (!tmp)
        return 0;

    buf->data = tmp;
    memcpy(buf->data + buf->len, ptr, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
    return n;
}

static int http_get(const char *url, struct buffer *out, long *status)
{
    CURL *curl = curl_easy_init();
    CURLcode rc;
    struct curl_slist *headers = NULL;

    if (!curl)
        return -1;

    out->data = NULL;
    out->len = 0;

    headers = curl_slist_append(
        headers,
        "Accept: application/rdap+json, application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 8L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "rdap-cli/" VERSION);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");

    rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        fprintf(stderr, "rdap: HTTP request failed: %s\n",
                curl_easy_strerror(rc));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(out->data);
        out->data = NULL;
        out->len = 0;
        return -1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return 0;
}

static json_object *fetch_json(const char *url)
{
    struct buffer buf = {0};
    long status = 0;
    json_object *obj;

    if (http_get(url, &buf, &status) != 0)
        return NULL;

    if (status < 200 || status >= 300) {
        fprintf(stderr, "rdap: bootstrap request returned HTTP %ld\n", status);
        free(buf.data);
        return NULL;
    }

    obj = json_tokener_parse(buf.data ? buf.data : "");
    free(buf.data);

    if (!obj)
        fprintf(stderr, "rdap: invalid JSON from %s\n", url);

    return obj;
}

static char *strdup_lower(const char *s)
{
    char *r = strdup(s);

    if (!r)
        return NULL;

    for (char *p = r; *p; ++p)
        *p = (char)tolower((unsigned char)*p);

    return r;
}

static enum object_type classify(const char *query, char **normalized)
{
    unsigned char addr[16];
    const char *p = query;

    *normalized = NULL;

    if (!strncasecmp(p, "AS", 2) && isdigit((unsigned char)p[2])) {
        p += 2;

        for (const char *q = p; *q; ++q) {
            if (!isdigit((unsigned char)*q))
                return OBJ_UNKNOWN;
        }

        *normalized = strdup(p);
        return *normalized ? OBJ_ASN : OBJ_UNKNOWN;
    }

    if (inet_pton(AF_INET, query, addr) == 1) {
        *normalized = strdup(query);
        return *normalized ? OBJ_IPV4 : OBJ_UNKNOWN;
    }

    if (inet_pton(AF_INET6, query, addr) == 1) {
        *normalized = strdup_lower(query);
        return *normalized ? OBJ_IPV6 : OBJ_UNKNOWN;
    }

    {
        int all_digits = 1;

        for (const char *q = query; *q; ++q) {
            if (!isdigit((unsigned char)*q)) {
                all_digits = 0;
                break;
            }
        }

        if (all_digits && *query) {
            *normalized = strdup(query);
            return *normalized ? OBJ_ASN : OBJ_UNKNOWN;
        }
    }

    if (strchr(query, '.')) {
        *normalized = strdup_lower(query);

        if (*normalized) {
            size_t n = strlen(*normalized);

            while (n && (*normalized)[n - 1] == '.')
                (*normalized)[--n] = '\0';
        }

        return *normalized && **normalized ? OBJ_DOMAIN : OBJ_UNKNOWN;
    }

    return OBJ_UNKNOWN;
}

static const char *pick_https_url(json_object *urls)
{
    size_t n = json_object_array_length(urls);

    for (size_t i = 0; i < n; ++i) {
        json_object *v = json_object_array_get_idx(urls, i);
        const char *s = json_object_get_string(v);

        if (s && !strncasecmp(s, "https://", 8))
            return s;
    }

    return NULL;
}

static int domain_match_len(const char *domain, const char *entry)
{
    size_t dl = strlen(domain);
    size_t el = strlen(entry);

    if (el > dl)
        return -1;

    if (strcasecmp(domain + dl - el, entry) != 0)
        return -1;

    if (dl == el || domain[dl - el - 1] == '.')
        return (int)el;

    return -1;
}

static char *discover_domain(json_object *root, const char *domain,
                             char **matched)
{
    json_object *services;
    const char *best = NULL;
    const char *best_entry = NULL;
    int best_len = -1;

    if (!json_object_object_get_ex(root, "services", &services))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(services); ++i) {
        json_object *svc = json_object_array_get_idx(services, i);
        json_object *entries = json_object_array_get_idx(svc, 0);
        json_object *urls = json_object_array_get_idx(svc, 1);

        if (!entries || !urls)
            continue;

        for (size_t j = 0; j < json_object_array_length(entries); ++j) {
            const char *entry =
                json_object_get_string(json_object_array_get_idx(entries, j));
            int ml = entry ? domain_match_len(domain, entry) : -1;

            if (ml > best_len) {
                const char *url = pick_https_url(urls);

                if (url) {
                    best = url;
                    best_entry = entry;
                    best_len = ml;
                }
            }
        }
    }

    if (matched && best_entry)
        *matched = strdup(best_entry);

    return best ? strdup(best) : NULL;
}

static int prefix_match(const unsigned char *addr,
                        const unsigned char *net,
                        int bits)
{
    int full = bits / 8;
    int rem = bits % 8;

    if (full && memcmp(addr, net, (size_t)full) != 0)
        return 0;

    if (rem) {
        unsigned char mask = (unsigned char)(0xff << (8 - rem));

        if ((addr[full] & mask) != (net[full] & mask))
            return 0;
    }

    return 1;
}

static char *discover_ip(json_object *root, const char *query, int af,
                         char **matched)
{
    json_object *services;
    unsigned char addr[16];
    const char *best = NULL;
    const char *best_entry = NULL;
    int best_bits = -1;

    if (inet_pton(af, query, addr) != 1)
        return NULL;

    if (!json_object_object_get_ex(root, "services", &services))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(services); ++i) {
        json_object *svc = json_object_array_get_idx(services, i);
        json_object *entries = json_object_array_get_idx(svc, 0);
        json_object *urls = json_object_array_get_idx(svc, 1);

        if (!entries || !urls)
            continue;

        for (size_t j = 0; j < json_object_array_length(entries); ++j) {
            const char *cidr =
                json_object_get_string(json_object_array_get_idx(entries, j));
            char copy[128];
            char *slash;
            char *end;
            long bits;
            unsigned char net[16];
            int maxbits = af == AF_INET ? 32 : 128;

            if (!cidr || strlen(cidr) >= sizeof(copy))
                continue;

            strcpy(copy, cidr);
            slash = strchr(copy, '/');

            if (!slash)
                continue;

            *slash++ = '\0';

            errno = 0;
            bits = strtol(slash, &end, 10);

            if (errno || *slash == '\0' || *end != '\0' ||
                bits < 0 || bits > maxbits)
                continue;

            if (inet_pton(af, copy, net) != 1)
                continue;

            if ((int)bits > best_bits &&
                prefix_match(addr, net, (int)bits)) {
                const char *url = pick_https_url(urls);

                if (url) {
                    best = url;
                    best_entry = cidr;
                    best_bits = (int)bits;
                }
            }
        }
    }

    if (matched && best_entry)
        *matched = strdup(best_entry);

    return best ? strdup(best) : NULL;
}

static int parse_u32(const char *s, uint32_t *out)
{
    char *end;
    unsigned long long v;

    errno = 0;
    v = strtoull(s, &end, 10);

    if (errno || *s == '\0' || *end != '\0' || v > UINT32_MAX)
        return -1;

    *out = (uint32_t)v;
    return 0;
}

static char *discover_asn(json_object *root, const char *query, char **matched)
{
    json_object *services;
    uint32_t asn;

    if (parse_u32(query, &asn) != 0)
        return NULL;

    if (!json_object_object_get_ex(root, "services", &services))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(services); ++i) {
        json_object *svc = json_object_array_get_idx(services, i);
        json_object *entries = json_object_array_get_idx(svc, 0);
        json_object *urls = json_object_array_get_idx(svc, 1);

        if (!entries || !urls)
            continue;

        for (size_t j = 0; j < json_object_array_length(entries); ++j) {
            const char *range =
                json_object_get_string(json_object_array_get_idx(entries, j));
            char copy[64];
            char *dash;
            uint32_t lo;
            uint32_t hi;

            if (!range || strlen(range) >= sizeof(copy))
                continue;

            strcpy(copy, range);
            dash = strchr(copy, '-');

            if (!dash)
                continue;

            *dash++ = '\0';

            if (parse_u32(copy, &lo) != 0 ||
                parse_u32(dash, &hi) != 0)
                continue;

            if (asn >= lo && asn <= hi) {
                const char *url = pick_https_url(urls);

                if (url) {
                    if (matched)
                        *matched = strdup(range);
                    return strdup(url);
                }
            }
        }
    }

    return NULL;
}

static char *join_url(const char *base, const char *kind, const char *query)
{
    size_t bl = strlen(base);
    int slash = bl && base[bl - 1] == '/';
    size_t need = bl + strlen(kind) + strlen(query) + 3;
    char *url = malloc(need);

    if (!url)
        return NULL;

    snprintf(url, need, "%s%s%s/%s",
             base, slash ? "" : "/", kind, query);

    return url;
}

static const char *jstr(json_object *obj, const char *key)
{
    json_object *v;

    if (json_object_object_get_ex(obj, key, &v) &&
        json_object_is_type(v, json_type_string))
        return json_object_get_string(v);

    return NULL;
}

static const char *find_related_rdap_link(json_object *obj)
{
    json_object *links;

    if (!json_object_object_get_ex(obj, "links", &links) ||
        !json_object_is_type(links, json_type_array))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(links); ++i) {
        json_object *link = json_object_array_get_idx(links, i);
        const char *rel = jstr(link, "rel");
        const char *href = jstr(link, "href");
        const char *type = jstr(link, "type");

        if (!rel || !href)
            continue;

        if (strcasecmp(rel, "related") != 0)
            continue;

        if (strncasecmp(href, "https://", 8) != 0)
            continue;

        if (type && strcasecmp(type, "application/rdap+json") != 0)
            continue;

        return href;
    }

    return NULL;
}


static int jbool(json_object *obj, const char *key, int *value)
{
    json_object *v;

    if (!json_object_object_get_ex(obj, key, &v))
        return 0;

    if (!json_object_is_type(v, json_type_boolean))
        return 0;

    *value = json_object_get_boolean(v);
    return 1;
}

static void print_string_field(json_object *obj,
                               const char *key,
                               const char *label)
{
    const char *s = jstr(obj, key);

    if (s)
        printf("%s: %s\n", label, s);
}

static void print_integer_field(json_object *obj,
                                const char *key,
                                const char *label)
{
    json_object *v;

    if (json_object_object_get_ex(obj, key, &v) &&
        (json_object_is_type(v, json_type_int) ||
         json_object_is_type(v, json_type_double))) {
        printf("%s: %s\n", label,
               json_object_to_json_string_ext(v, JSON_C_TO_STRING_PLAIN));
    }
}

static void print_status(json_object *obj)
{
    json_object *arr;

    if (!json_object_object_get_ex(obj, "status", &arr) ||
        !json_object_is_type(arr, json_type_array) ||
        json_object_array_length(arr) == 0)
        return;

    fputs("Status: ", stdout);

    for (size_t i = 0; i < json_object_array_length(arr); ++i) {
        const char *s =
            json_object_get_string(json_object_array_get_idx(arr, i));

        if (i)
            fputs(", ", stdout);

        fputs(s ? s : "", stdout);
    }

    putchar('\n');
}

static const char *event_label(const char *action)
{
    if (!action)
        return "";

    if (!strcasecmp(action, "registration"))
        return "Registration";

    if (!strcasecmp(action, "expiration"))
        return "Expiration";

    if (!strcasecmp(action, "last changed"))
        return "Last changed";

    if (!strcasecmp(action, "last update of RDAP database"))
        return "RDAP database updated";

    return action;
}

static void print_events(json_object *obj)
{
    json_object *events;

    if (!json_object_object_get_ex(obj, "events", &events) ||
        !json_object_is_type(events, json_type_array) ||
        json_object_array_length(events) == 0)
        return;

    puts("Events:");

    for (size_t i = 0; i < json_object_array_length(events); ++i) {
        json_object *event = json_object_array_get_idx(events, i);
        const char *action = jstr(event, "eventAction");
        const char *date = jstr(event, "eventDate");

        if (action || date)
            printf("  %-24s %s\n",
                   event_label(action),
                   date ? date : "");
    }
}

static void print_nameservers(json_object *obj)
{
    json_object *arr;

    if (!json_object_object_get_ex(obj, "nameservers", &arr) ||
        !json_object_is_type(arr, json_type_array) ||
        json_object_array_length(arr) == 0)
        return;

    puts("Nameservers:");

    for (size_t i = 0; i < json_object_array_length(arr); ++i) {
        json_object *ns = json_object_array_get_idx(arr, i);
        const char *name = jstr(ns, "ldhName");

        if (!name)
            name = jstr(ns, "unicodeName");

        if (name)
            printf("  %s\n", name);
    }
}

static int entity_has_role(json_object *entity, const char *wanted)
{
    json_object *roles;

    if (!json_object_object_get_ex(entity, "roles", &roles) ||
        !json_object_is_type(roles, json_type_array))
        return 0;

    for (size_t i = 0; i < json_object_array_length(roles); ++i) {
        const char *role =
            json_object_get_string(json_object_array_get_idx(roles, i));

        if (role && !strcasecmp(role, wanted))
            return 1;
    }

    return 0;
}

/*
 * RDAP uses jCard (RFC 7095) in vcardArray.
 * Return the first string value for a property such as "fn", "org",
 * "email", or "tel".
 */
static const char *vcard_property(json_object *entity, const char *wanted)
{
    json_object *vcard;
    json_object *props;

    if (!json_object_object_get_ex(entity, "vcardArray", &vcard) ||
        !json_object_is_type(vcard, json_type_array) ||
        json_object_array_length(vcard) < 2)
        return NULL;

    props = json_object_array_get_idx(vcard, 1);

    if (!props || !json_object_is_type(props, json_type_array))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(props); ++i) {
        json_object *prop = json_object_array_get_idx(props, i);
        json_object *name;
        json_object *value;
        const char *pname;

        if (!prop || !json_object_is_type(prop, json_type_array) ||
            json_object_array_length(prop) < 4)
            continue;

        name = json_object_array_get_idx(prop, 0);
        value = json_object_array_get_idx(prop, 3);
        pname = json_object_get_string(name);

        if (!pname || strcasecmp(pname, wanted) != 0)
            continue;

        if (json_object_is_type(value, json_type_string))
            return json_object_get_string(value);
    }

    return NULL;
}

static const char *entity_display_name(json_object *entity)
{
    const char *s;

    s = vcard_property(entity, "fn");
    if (s && *s)
        return s;

    s = vcard_property(entity, "org");
    if (s && *s)
        return s;

    return jstr(entity, "handle");
}


static int contains_case_insensitive(const char *haystack, const char *needle)
{
    size_t nl;

    if (!haystack || !needle)
        return 0;

    nl = strlen(needle);

    if (nl == 0)
        return 1;

    for (const char *p = haystack; *p; ++p) {
        if (!strncasecmp(p, needle, nl))
            return 1;
    }

    return 0;
}

static const char *entity_public_id(json_object *entity,
                                    const char *type_substring)
{
    json_object *ids;

    if (!json_object_object_get_ex(entity, "publicIds", &ids) ||
        !json_object_is_type(ids, json_type_array))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(ids); ++i) {
        json_object *id = json_object_array_get_idx(ids, i);
        const char *type = jstr(id, "type");
        const char *identifier = jstr(id, "identifier");

        if (type && identifier &&
            contains_case_insensitive(type, type_substring))
            return identifier;
    }

    return NULL;
}

static json_object *find_entity_by_role(json_object *obj, const char *role)
{
    json_object *entities;

    if (!json_object_object_get_ex(obj, "entities", &entities) ||
        !json_object_is_type(entities, json_type_array))
        return NULL;

    for (size_t i = 0; i < json_object_array_length(entities); ++i) {
        json_object *entity = json_object_array_get_idx(entities, i);
        json_object *nested;

        if (entity_has_role(entity, role))
            return entity;

        nested = find_entity_by_role(entity, role);
        if (nested)
            return nested;
    }

    return NULL;
}

static void print_registrar(json_object *obj)
{
    json_object *entity = find_entity_by_role(obj, "registrar");
    const char *name;
    const char *iana;

    if (!entity)
        return;

    name = entity_display_name(entity);
    iana = entity_public_id(entity, "iana");

    puts("Registrar:");

    if (name)
        printf("  Name: %s\n", name);

    if (iana)
        printf("  IANA ID: %s\n", iana);

    if (!name && !iana) {
        const char *handle = jstr(entity, "handle");

        if (handle)
            printf("  Handle: %s\n", handle);
    }
}

static void print_abuse_contact(json_object *obj)
{
    json_object *entity = find_entity_by_role(obj, "abuse");
    const char *name;
    const char *email;
    const char *tel;

    if (!entity)
        return;

    name = entity_display_name(entity);
    email = vcard_property(entity, "email");
    tel = vcard_property(entity, "tel");

    if (!name && !email && !tel)
        return;

    puts("Abuse Contact:");

    if (name)
        printf("  Name: %s\n", name);

    if (email)
        printf("  Email: %s\n", email);
    if (tel) {
        if (!strncasecmp(tel, "tel:", 4))
            tel += 4;

        printf("  Phone: %s\n", tel);
    }
}

static void print_dnssec(json_object *obj)
{
    json_object *secure;
    json_object *ds;
    int signed_value;
    int have_output = 0;

    if (!json_object_object_get_ex(obj, "secureDNS", &secure) ||
        !json_object_is_type(secure, json_type_object))
        return;

    if (jbool(secure, "delegationSigned", &signed_value)) {
        puts("DNSSEC:");
        printf("  Delegation signed: %s\n",
               signed_value ? "yes" : "no");
        have_output = 1;
    }

    if (json_object_object_get_ex(secure, "dsData", &ds) &&
        json_object_is_type(ds, json_type_array) &&
        json_object_array_length(ds) > 0) {

        if (!have_output) {
            puts("DNSSEC:");
            have_output = 1;
        }

        puts("  DS:");

        for (size_t i = 0; i < json_object_array_length(ds); ++i) {
            json_object *entry = json_object_array_get_idx(ds, i);
            json_object *v;

            if (i)
                putchar('\n');

            if (json_object_object_get_ex(entry, "keyTag", &v))
                printf("    Key tag: %s\n",
                       json_object_to_json_string_ext(
                           v, JSON_C_TO_STRING_PLAIN));

            if (json_object_object_get_ex(entry, "algorithm", &v))
                printf("    Algorithm: %s\n",
                       json_object_to_json_string_ext(
                           v, JSON_C_TO_STRING_PLAIN));

            if (json_object_object_get_ex(entry, "digestType", &v))
                printf("    Digest type: %s\n",
                       json_object_to_json_string_ext(
                           v, JSON_C_TO_STRING_PLAIN));

            if (json_object_object_get_ex(entry, "digest", &v)) {
                const char *digest = json_object_get_string(v);

                if (digest)
                    printf("    Digest: %s\n", digest);
            }
        }
    }

    (void)have_output;
}

static void print_notices(json_object *obj)
{
    json_object *notices;

    if (!json_object_object_get_ex(obj, "notices", &notices) ||
        !json_object_is_type(notices, json_type_array) ||
        json_object_array_length(notices) == 0)
        return;

    puts("Notices:");

    for (size_t i = 0; i < json_object_array_length(notices); ++i) {
        json_object *notice = json_object_array_get_idx(notices, i);
        json_object *desc;
        const char *title = jstr(notice, "title");

        if (title)
            printf("  %s\n", title);
        else
            printf("  Notice %zu\n", i + 1);

        if (json_object_object_get_ex(notice, "description", &desc) &&
            json_object_is_type(desc, json_type_array)) {
            for (size_t j = 0; j < json_object_array_length(desc); ++j) {
                const char *line =
                    json_object_get_string(
                        json_object_array_get_idx(desc, j));

                if (line)
                    printf("    %s\n", line);
            }
        }
    }
}

static void print_link_array(json_object *links, const char *indent)
{
    if (!links || !json_object_is_type(links, json_type_array))
        return;

    for (size_t i = 0; i < json_object_array_length(links); ++i) {
        json_object *link = json_object_array_get_idx(links, i);
        const char *rel = jstr(link, "rel");
        const char *href = jstr(link, "href");

        if (!href)
            continue;

        if (rel)
            printf("%s%-12s %s\n", indent, rel, href);
        else
            printf("%s%s\n", indent, href);
    }
}

static void print_links(json_object *obj)
{
    json_object *links;

    if (!json_object_object_get_ex(obj, "links", &links) ||
        !json_object_is_type(links, json_type_array) ||
        json_object_array_length(links) == 0)
        return;

    puts("Links:");
    print_link_array(links, "  ");
}

static void print_summary(json_object *obj,
                          int include_notices,
                          int include_links)
{
    const char *kind = jstr(obj, "objectClassName");
    const char *handle = jstr(obj, "handle");
    const char *ldh = jstr(obj, "ldhName");
    const char *unicode = jstr(obj, "unicodeName");
    const char *name = jstr(obj, "name");

    if (kind)
        printf("Type: %s\n", kind);

    if (ldh)
        printf("Name: %s\n", ldh);
    else if (name)
        printf("Name: %s\n", name);

    if (unicode && (!ldh || strcmp(unicode, ldh) != 0))
        printf("Unicode Name: %s\n", unicode);

    if (handle)
        printf("Handle: %s\n", handle);

    print_string_field(obj, "type", "Registration Type");
    print_string_field(obj, "country", "Country");
    print_string_field(obj, "ipVersion", "IP Version");
    print_string_field(obj, "parentHandle", "Parent Handle");
    print_string_field(obj, "startAddress", "Start Address");
    print_string_field(obj, "endAddress", "End Address");
    print_integer_field(obj, "startAutnum", "Start ASN");
    print_integer_field(obj, "endAutnum", "End ASN");

    print_status(obj);
    print_registrar(obj);
    print_abuse_contact(obj);
    print_events(obj);
    print_nameservers(obj);
    print_dnssec(obj);

    if (include_notices)
        print_notices(obj);

    if (include_links)
        print_links(obj);
}

int main(int argc, char **argv)
{
    int raw_json = 0;
    int include_notices = 0;
    int include_links = 0;
    int follow_related = 0;
    int server_only = 0;
    int verbose = 0;
    const char *query = NULL;
    char *normalized = NULL;
    char *base = NULL;
    char *matched = NULL;
    char *url = NULL;
    const char *bootstrap = NULL;
    const char *kind = NULL;
    enum object_type type;
    json_object *boot = NULL;
    json_object *answer = NULL;
    json_object *related_answer = NULL;
    struct buffer response = {0};
    struct buffer related_response = {0};
    long status = 0;
    long related_status = 0;
    int rc = 1;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-j") || !strcmp(argv[i], "--json")) {
            raw_json = 1;
        } else if (!strcmp(argv[i], "-n") ||
                   !strcmp(argv[i], "--notices")) {
            include_notices = 1;
        } else if (!strcmp(argv[i], "-l") ||
                   !strcmp(argv[i], "--links")) {
            include_links = 1;
        } else if (!strcmp(argv[i], "-s") ||
                   !strcmp(argv[i], "--server")) {
            server_only = 1;
        } else if (!strcmp(argv[i], "-f") ||
                   !strcmp(argv[i], "--follow")) {
            follow_related = 1;
        } else if (!strcmp(argv[i], "-v") ||
                   !strcmp(argv[i], "--verbose")) {
            verbose = 1;
        } else if (!strcmp(argv[i], "-V") ||
                   !strcmp(argv[i], "--version")) {
            printf("rdap %s\n", VERSION);
            return 0;
        } else if (!strcmp(argv[i], "-h") ||
                   !strcmp(argv[i], "--help")) {
            usage(stdout, argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "rdap: unknown option: %s\n", argv[i]);
            usage(stderr, argv[0]);
            return 2;
        } else if (query) {
            fprintf(stderr, "rdap: expected exactly one query\n");
            return 2;
        } else {
            query = argv[i];
        }
    }

    if (!query) {
        usage(stderr, argv[0]);
        return 2;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "rdap: failed to initialize libcurl\n");
        return 1;
    }

    type = classify(query, &normalized);

    switch (type) {
    case OBJ_DOMAIN:
        bootstrap = IANA_DNS;
        kind = "domain";
        break;
    case OBJ_IPV4:
        bootstrap = IANA_IPV4;
        kind = "ip";
        break;
    case OBJ_IPV6:
        bootstrap = IANA_IPV6;
        kind = "ip";
        break;
    case OBJ_ASN:
        bootstrap = IANA_ASN;
        kind = "autnum";
        break;
    default:
        fprintf(stderr,
            "rdap: cannot determine query type; "
            "use a domain, IP address, or ASN\n");
        goto done;
    }

    if (verbose) {
        fprintf(stderr, "rdap: query type: %s\n", kind);
        fprintf(stderr, "rdap: bootstrap: %s\n", bootstrap);
    }

    boot = fetch_json(bootstrap);
    if (!boot)
        goto done;

    switch (type) {
    case OBJ_DOMAIN:
        base = discover_domain(boot, normalized, &matched);
        break;
    case OBJ_IPV4:
        base = discover_ip(boot, normalized, AF_INET, &matched);
        break;
    case OBJ_IPV6:
        base = discover_ip(boot, normalized, AF_INET6, &matched);
        break;
    case OBJ_ASN:
        base = discover_asn(boot, normalized, &matched);
        break;
    default:
        break;
    }

    if (!base) {
        fprintf(stderr, "rdap: no authoritative RDAP service found\n");
        goto done;
    }

    if (strncasecmp(base, "https://", 8) != 0) {
        fprintf(stderr,
                "rdap: refusing non-HTTPS RDAP service: %s\n",
                base);
        goto done;
    }

    if (verbose) {
        if (matched)
            fprintf(stderr, "rdap: bootstrap match: %s\n", matched);

        fprintf(stderr, "rdap: RDAP server: %s\n", base);
    }

    if (server_only) {
        puts(base);
        rc = 0;
        goto done;
    }

    url = join_url(base, kind, normalized);

    if (!url) {
        fprintf(stderr, "rdap: out of memory\n");
        goto done;
    }

    if (verbose)
        fprintf(stderr, "rdap: request: %s\n", url);

    if (http_get(url, &response, &status) != 0)
        goto done;

    if (status < 200 || status >= 300) {
        fprintf(stderr, "rdap: server returned HTTP %ld\n", status);

        if (response.data && response.len)
            fprintf(stderr, "%s\n", response.data);

        goto done;
    }

    answer = json_tokener_parse(response.data ? response.data : "");

    if (!answer) {
        fprintf(stderr, "rdap: server returned invalid JSON\n");
        goto done;
    }

    if (follow_related) {
        const char *related = find_related_rdap_link(answer);

        if (related) {
            if (verbose)
                fprintf(stderr,
                        "rdap: following related RDAP link: %s\n",
                        related);

            if (http_get(related,
                         &related_response,
                         &related_status) == 0) {
                if (related_status >= 200 && related_status < 300) {
                    related_answer =
                        json_tokener_parse(
                            related_response.data ?
                            related_response.data : "");

                    if (!related_answer) {
                        fprintf(stderr,
                                "rdap: related server returned invalid JSON\n");
                    }
                } else {
                    fprintf(stderr,
                            "rdap: related RDAP server returned HTTP %ld\n",
                            related_status);
                }
            }
        } else if (verbose) {
            fprintf(stderr,
                    "rdap: no related registrar RDAP link found\n");
        }
    }

    if (raw_json) {
        if (follow_related) {
            json_object *combined = json_object_new_object();

            if (!combined) {
                fprintf(stderr, "rdap: out of memory\n");
                goto done;
            }

            /*
             * json_object_object_add() takes ownership of values, so take
             * our own references here. The originals remain owned by main().
             */
            json_object_object_add(combined,
                                   "registry",
                                   json_object_get(answer));

            if (related_answer) {
                json_object_object_add(combined,
                                       "registrar",
                                       json_object_get(related_answer));
            } else {
                json_object_object_add(combined,
                                       "registrar",
                                       json_object_new_null());
            }

            puts(json_object_to_json_string_ext(
                combined,
                JSON_C_TO_STRING_PRETTY |
                JSON_C_TO_STRING_NOSLASHESCAPE));

            json_object_put(combined);
        } else {
            puts(json_object_to_json_string_ext(
                answer,
                JSON_C_TO_STRING_PRETTY |
                JSON_C_TO_STRING_NOSLASHESCAPE));
        }
    } else {
        print_summary(answer, include_notices, include_links);

        if (follow_related && related_answer) {
            puts("\nRegistrar RDAP:");
            print_summary(related_answer,
                          include_notices,
                          include_links);
        }
    }

    rc = 0;

done:
    json_object_put(related_answer);
    json_object_put(answer);
    json_object_put(boot);
    free(related_response.data);
    free(response.data);
    free(url);
    free(matched);
    free(base);
    free(normalized);
    curl_global_cleanup();
    return rc;
}


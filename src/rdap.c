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

#define VERSION "0.1.0"

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
        "  %s 8.8.8.8\n"
        "  %s 2001:4860:4860::8888\n"
        "  %s AS15169\n"
        "  %s --json example.com\n"
        "\n"
        "Options:\n"
        "  -j, --json       Print raw RDAP JSON\n"
        "  -v, --verbose    Show discovery details on stderr\n"
        "  -V, --version    Print version and exit\n"
        "  -h, --help       Show this help\n",
        prog, prog, prog, prog, prog, prog);
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

    headers = curl_slist_append(headers, "Accept: application/rdap+json, application/json");
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
        for (const char *q = p; *q; ++q)
            if (!isdigit((unsigned char)*q))
                return OBJ_UNKNOWN;
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

static const char *pick_https_url(json_object *urls)
{
    size_t n = json_object_array_length(urls);
    const char *fallback = NULL;

    for (size_t i = 0; i < n; ++i) {
        json_object *v = json_object_array_get_idx(urls, i);
        const char *s = json_object_get_string(v);
        if (!s)
            continue;
        if (!fallback)
            fallback = s;
        if (!strncasecmp(s, "https://", 8))
            return s;
    }
    return fallback;
}

static int domain_match_len(const char *domain, const char *entry)
{
    size_t dl = strlen(domain), el = strlen(entry);

    if (el > dl)
        return -1;
    if (strcasecmp(domain + dl - el, entry) != 0)
        return -1;
    if (dl == el || domain[dl - el - 1] == '.')
        return (int)el;
    return -1;
}

static char *discover_domain(json_object *root, const char *domain)
{
    json_object *services;
    const char *best = NULL;
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
                const char *u = pick_https_url(urls);
                if (u) {
                    best = u;
                    best_len = ml;
                }
            }
        }
    }

    return best ? strdup(best) : NULL;
}

static int prefix_match(const unsigned char *addr, const unsigned char *net,
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

static char *discover_ip(json_object *root, const char *query, int af)
{
    json_object *services;
    unsigned char addr[16];
    const char *best = NULL;
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
            bits = strtol(slash, NULL, 10);
            if (errno || bits < 0 || bits > maxbits)
                continue;

            if (inet_pton(af, copy, net) != 1)
                continue;

            if ((int)bits > best_bits && prefix_match(addr, net, (int)bits)) {
                const char *u = pick_https_url(urls);
                if (u) {
                    best = u;
                    best_bits = (int)bits;
                }
            }
        }
    }

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

static char *discover_asn(json_object *root, const char *query)
{
    json_object *services;
    uint32_t asn;
    const char *best = NULL;

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
            char copy[64], *dash;
            uint32_t lo, hi;

            if (!range || strlen(range) >= sizeof(copy))
                continue;

            strcpy(copy, range);
            dash = strchr(copy, '-');
            if (!dash)
                continue;

            *dash++ = '\0';
            if (parse_u32(copy, &lo) != 0 || parse_u32(dash, &hi) != 0)
                continue;

            if (asn >= lo && asn <= hi) {
                best = pick_https_url(urls);
                if (best)
                    return strdup(best);
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

    snprintf(url, need, "%s%s%s/%s", base, slash ? "" : "/", kind, query);
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

static void print_events(json_object *obj)
{
    json_object *events;

    if (!json_object_object_get_ex(obj, "events", &events) ||
        !json_object_is_type(events, json_type_array))
        return;

    puts("Events:");
    for (size_t i = 0; i < json_object_array_length(events); ++i) {
        json_object *e = json_object_array_get_idx(events, i);
        const char *action = jstr(e, "eventAction");
        const char *date = jstr(e, "eventDate");
        if (action || date)
            printf("  %-22s %s\n", action ? action : "", date ? date : "");
    }
}

static void print_status(json_object *obj)
{
    json_object *arr;

    if (!json_object_object_get_ex(obj, "status", &arr) ||
        !json_object_is_type(arr, json_type_array))
        return;

    fputs("Status: ", stdout);
    for (size_t i = 0; i < json_object_array_length(arr); ++i) {
        if (i) fputs(", ", stdout);
        fputs(json_object_get_string(json_object_array_get_idx(arr, i)), stdout);
    }
    putchar('\n');
}

static void print_nameservers(json_object *obj)
{
    json_object *arr;

    if (!json_object_object_get_ex(obj, "nameservers", &arr) ||
        !json_object_is_type(arr, json_type_array))
        return;

    puts("Nameservers:");
    for (size_t i = 0; i < json_object_array_length(arr); ++i) {
        json_object *ns = json_object_array_get_idx(arr, i);
        const char *name = jstr(ns, "ldhName");
        if (name)
            printf("  %s\n", name);
    }
}

static void print_summary(json_object *obj)
{
    const char *kind = jstr(obj, "objectClassName");
    const char *handle = jstr(obj, "handle");
    const char *ldh = jstr(obj, "ldhName");
    const char *name = jstr(obj, "name");
    const char *start = jstr(obj, "startAddress");
    const char *end = jstr(obj, "endAddress");

    if (kind)   printf("Type: %s\n", kind);
    if (ldh)    printf("Name: %s\n", ldh);
    else if (name) printf("Name: %s\n", name);
    if (handle) printf("Handle: %s\n", handle);
    if (start)  printf("Start address: %s\n", start);
    if (end)    printf("End address: %s\n", end);

    {
        json_object *start_asn, *end_asn;
        if (json_object_object_get_ex(obj, "startAutnum", &start_asn))
            printf("Start ASN: %s\n", json_object_to_json_string_ext(
                start_asn, JSON_C_TO_STRING_PLAIN));
        if (json_object_object_get_ex(obj, "endAutnum", &end_asn))
            printf("End ASN: %s\n", json_object_to_json_string_ext(
                end_asn, JSON_C_TO_STRING_PLAIN));
    }

    print_status(obj);
    print_events(obj);
    print_nameservers(obj);
}

int main(int argc, char **argv)
{
    int raw_json = 0, verbose = 0;
    const char *query = NULL;
    char *normalized = NULL;
    char *base = NULL;
    char *url = NULL;
    const char *bootstrap = NULL;
    const char *kind = NULL;
    enum object_type type;
    json_object *boot = NULL, *answer = NULL;
    struct buffer response = {0};
    long status = 0;
    int rc = 1;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-j") || !strcmp(argv[i], "--json"))
            raw_json = 1;
        else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
            verbose = 1;
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            printf("rdap %s\n", VERSION);
            return 0;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
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
            "rdap: cannot determine query type; use a domain, IP address, or ASN\n");
        goto done;
    }

    if (verbose)
        fprintf(stderr, "rdap: bootstrap: %s\n", bootstrap);

    boot = fetch_json(bootstrap);
    if (!boot)
        goto done;

    switch (type) {
    case OBJ_DOMAIN:
        base = discover_domain(boot, normalized);
        break;
    case OBJ_IPV4:
        base = discover_ip(boot, normalized, AF_INET);
        break;
    case OBJ_IPV6:
        base = discover_ip(boot, normalized, AF_INET6);
        break;
    case OBJ_ASN:
        base = discover_asn(boot, normalized);
        break;
    default:
        break;
    }

    if (!base) {
        fprintf(stderr, "rdap: no authoritative RDAP service found\n");
        goto done;
    }

    if (strncasecmp(base, "https://", 8) != 0) {
        fprintf(stderr, "rdap: refusing non-HTTPS RDAP service: %s\n", base);
        goto done;
    }

    url = join_url(base, kind, normalized);
    if (!url) {
        fprintf(stderr, "rdap: out of memory\n");
        goto done;
    }

    if (verbose)
        fprintf(stderr, "rdap: query: %s\n", url);

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

    if (raw_json) {
        puts(json_object_to_json_string_ext(answer,
             JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
    } else {
        print_summary(answer);
    }

    rc = 0;

done:
    json_object_put(answer);
    json_object_put(boot);
    free(response.data);
    free(url);
    free(base);
    free(normalized);
    curl_global_cleanup();
    return rc;
}

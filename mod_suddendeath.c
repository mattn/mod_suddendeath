#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include <http_log.h>
#include <stdlib.h>
#include <wchar.h>

struct _interval {
  unsigned short first;
  unsigned short last;
};

static int _bisearch(wchar_t ucs, const struct _interval *table, int max) {
  int min = 0;
  int mid;
  if (ucs < table[0].first || ucs > table[max].last)
    return 0;
  while (max >= min) {
    mid = (min + max) / 2;
    if (ucs > table[mid].last)
      min = mid + 1;
    else if (ucs < table[mid].first)
      max = mid - 1;
    else
      return 1;
  }
  return 0;
}

static int _wcwidth(wchar_t ucs) {
  static const struct _interval combining[] = {
    { 0x0300, 0x034E }, { 0x0360, 0x0362 }, { 0x0483, 0x0486 },
    { 0x0488, 0x0489 }, { 0x0591, 0x05A1 }, { 0x05A3, 0x05B9 },
    { 0x05BB, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
    { 0x05C4, 0x05C4 }, { 0x064B, 0x0655 }, { 0x0670, 0x0670 },
    { 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
    { 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
    { 0x07A6, 0x07B0 }, { 0x0901, 0x0902 }, { 0x093C, 0x093C },
    { 0x0941, 0x0948 }, { 0x094D, 0x094D }, { 0x0951, 0x0954 },
    { 0x0962, 0x0963 }, { 0x0981, 0x0981 }, { 0x09BC, 0x09BC },
    { 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD }, { 0x09E2, 0x09E3 },
    { 0x0A02, 0x0A02 }, { 0x0A3C, 0x0A3C }, { 0x0A41, 0x0A42 },
    { 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D }, { 0x0A70, 0x0A71 },
    { 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC }, { 0x0AC1, 0x0AC5 },
    { 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD }, { 0x0B01, 0x0B01 },
    { 0x0B3C, 0x0B3C }, { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 },
    { 0x0B4D, 0x0B4D }, { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 },
    { 0x0BC0, 0x0BC0 }, { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 },
    { 0x0C46, 0x0C48 }, { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 },
    { 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
    { 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D }, { 0x0DCA, 0x0DCA },
    { 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 }, { 0x0E31, 0x0E31 },
    { 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E }, { 0x0EB1, 0x0EB1 },
    { 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC }, { 0x0EC8, 0x0ECD },
    { 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 }, { 0x0F37, 0x0F37 },
    { 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E }, { 0x0F80, 0x0F84 },
    { 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 }, { 0x0F99, 0x0FBC },
    { 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 }, { 0x1032, 0x1032 },
    { 0x1036, 0x1037 }, { 0x1039, 0x1039 }, { 0x1058, 0x1059 },
    { 0x1160, 0x11FF }, { 0x17B7, 0x17BD }, { 0x17C6, 0x17C6 },
    { 0x17C9, 0x17D3 }, { 0x180B, 0x180E }, { 0x18A9, 0x18A9 },
    { 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x206A, 0x206F },
    { 0x20D0, 0x20E3 }, { 0x302A, 0x302F }, { 0x3099, 0x309A },
    { 0xFB1E, 0xFB1E }, { 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF },
    { 0xFFF9, 0xFFFB }
  };

  if (ucs == 0)
    return 0;
  if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
    return -1;

  if (_bisearch(ucs, combining,
      sizeof(combining) / sizeof(struct _interval) - 1))
    return 0;

  return 1 + 
    (ucs >= 0x1100 &&
     (ucs <= 0x115f ||                    /* Hangul Jamo init. consonants */
      (ucs >= 0x2e80 && ucs <= 0xa4cf && (ucs & ~0x0011) != 0x300a &&
       ucs != 0x303f) ||                  /* CJK ... Yi */
      (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
      (ucs >= 0xf900 && ucs <= 0xfaff) || /* CJK Compatibility Ideographs */
      (ucs >= 0xfe30 && ucs <= 0xfe6f) || /* CJK Compatibility Forms */
      (ucs >= 0xff00 && ucs <= 0xff5f) || /* Fullwidth Forms */
      (ucs >= 0xffe0 && ucs <= 0xffe6) ||
      (ucs >= 0x20000 && ucs <= 0x2ffff)));
}

static char utf8len_tab[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1,
};

static char utf8len_tab_zero[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,0,0,
};

static int utf_ptr2len(const unsigned char *p) {
  int        len;
  int        i;

  if (*p == 0)
    return 0;
  len = utf8len_tab[*p];
  for (i = 1; i < len; ++i)
    if ((p[i] & 0xc0) != 0x80)
      return 1;
  return len;
}

static int utf_ptr2char(const unsigned char *p) {
  int    len;
  if (p[0] < 0x80)
    return p[0];

  len = utf8len_tab_zero[p[0]];
  if (len > 1 && (p[1] & 0xc0) == 0x80) {
    if (len == 2)
      return ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
    if ((p[2] & 0xc0) == 0x80) {
      if (len == 3)
        return ((p[0] & 0x0f) << 12) + ((p[1] & 0x3f) << 6)
          + (p[2] & 0x3f);
      if ((p[3] & 0xc0) == 0x80) {
        if (len == 4)
          return ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12)
            + ((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
        if ((p[4] & 0xc0) == 0x80) {
          if (len == 5)
            return ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18)
              + ((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6)
              + (p[4] & 0x3f);
          if ((p[5] & 0xc0) == 0x80 && len == 6)
            return ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24)
              + ((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12)
              + ((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
        }
      }
    }
  }
  return p[0];
}

typedef struct {
  char* message;
} suddendeath_config;

module AP_MODULE_DECLARE_DATA suddendeath_module;

static int utf_strwidth(const char* s, int l) {
  int maxwidth = 0, width = 0, i;
  const unsigned char* p = s;
  while (*p) {
    if (*p == '\n')
      width = 0;
    else
      width += _wcwidth(utf_ptr2char(p));
    if (maxwidth < width)
      maxwidth = width;
	{
		FILE* fp = fopen("/tmp/foo.log", "a");
		fprintf(fp, "%s, %d, %d\n", p, width, utf_ptr2char(p));
		fclose(fp);
	}
    p += utf_ptr2len(p);
    if (l != -1 && (char*)p - (char*)s > l) break;
  }
  return maxwidth;
}

static int suddendeath_handler(request_rec* r) {
  suddendeath_config *conf = ap_get_module_config(r->per_dir_config, &suddendeath_module);
  char *message, *ptr;
  int maxwidth = 0, width = 0, i;
  if (!r->handler || strcmp(r->handler, "suddendeath"))
    return DECLINED ;
  if (r->method_number != M_GET)
    return HTTP_METHOD_NOT_ALLOWED ;
  ap_set_content_type(r, "text/html;charset=utf-8") ;
  ap_rputs("<html><head><meta charset=\"utf-8\"/><title>suddendeath!</title></head>\n", r);
  ap_rputs("<body><pre>\n", r);
  message = conf->message ? conf->message : "突然の死";
  maxwidth = utf_strwidth(message, -1);
  ap_rputs("＿", r);
  for (i = 0; i < maxwidth / 2; i++)
    ap_rputs("人", r);
  ap_rputs("＿\n", r);

  ap_rputs("＞", r);
  while (*message) {
    ptr = strchr(message, '\n');
    if (ptr) {
      ap_rwrite(message, ptr - message, r);
      width = utf_strwidth(message, ptr - message);
      for (i = 0; i < maxwidth - width; i++)
        ap_rputs(" ", r);
      ap_rputs("＜\n", r);
      message = ptr + 1;
      if (*message)
        ap_rputs("＞", r);
    } else {
      ap_rprintf(r, "%s", message);
      width = utf_strwidth(message, -1);
      for (i = 0; i < maxwidth - width; i++)
        ap_rputs(" ", r);
      ap_rputs("＜\n", r);
      break;
    }
  }

  ap_rputs("￣", r);
  for (i = 0; i < maxwidth / 2; i++)
    ap_rputs("Y^", r);
  ap_rputs("￣\n", r);

  ap_rputs("</pre></body></html>\n", r);
  return OK ;
}

static void suddendeath_hooks(apr_pool_t* pool) {
  ap_hook_handler(suddendeath_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

static void *create_suddendeath_dir_config(apr_pool_t *p, char *d) {
  suddendeath_config *conf = apr_palloc(p, sizeof(*conf));
  conf->message = NULL;
  return conf;
}

static const char *set_suddendeath_message(cmd_parms *cmd, void *in_dir_config, const char *message) {
  suddendeath_config *conf = in_dir_config;
  conf->message = (char*) message;
  return NULL;
}

static const char *set_suddendeath_file(cmd_parms *cmd, void *in_dir_config, const char *filename) {
  suddendeath_config *conf = in_dir_config;
  FILE* fp = fopen(filename, "rb");
  if (!fp) return NULL;
  int size = 0;
  if (!conf->message) {
    free(conf->message);
    conf->message = NULL;
  }
  while (!feof(fp)) {
    char buf[BUFSIZ] = {0};
    fgets(buf, sizeof(buf)-1, fp);
    size += strlen(buf);
    if (!conf->message) {
      conf->message = malloc(size+1);
      *conf->message = 0;
    } else
      conf->message = realloc(conf->message, size+1);
    strcat(conf->message, buf);
  }
  fclose(fp);
  return NULL;
}

static const command_rec suddendeath_cmds[] = {
  AP_INIT_TAKE1("SuddenDeathMessage", set_suddendeath_message, NULL, OR_LIMIT, "sudden death message"),
  AP_INIT_TAKE1("SuddenDeathFile", set_suddendeath_file, NULL, OR_LIMIT, "sudden death file"),
  { NULL }
};

module AP_MODULE_DECLARE_DATA suddendeath_module = {
  STANDARD20_MODULE_STUFF,
  create_suddendeath_dir_config,
  NULL,
  NULL,
  NULL,
  suddendeath_cmds,
  suddendeath_hooks
};

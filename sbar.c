#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

#include <linux/if_link.h>

char *utcusa = "America/Denver";

static Display *dpy;

char *smprintf(char *fmt, ...) {
  va_list fmtargs;
  char *ret;
  int len;

  va_start(fmtargs, fmt);
  len = vsnprintf(NULL, 0, fmt, fmtargs);
  va_end(fmtargs);

  ret = malloc(++len);
  if (ret == NULL) {
    perror("malloc");
    exit(1);
	}

  va_start(fmtargs, fmt);
  vsnprintf(ret, len, fmt, fmtargs);
  va_end(fmtargs);

  return ret;
}

char *get_status() {
  FILE *ac;
  char *plug = "/sys/class/power_supply/AC/online";
  if ((ac = fopen(plug, "r")) == NULL) {
    perror("ac error");
    exit(1);
  }
  char retVal = fgetc(ac);
  fclose(ac);

  FILE *bat;
  char *sta = "/sys/class/power_supply/BAT0/energy_now";
  if ((bat = fopen(sta, "r")) == NULL) {
    perror("energy_now error");
    exit(1);
  }
  char now[8];
  fgets(now, 8, bat);
  fclose(bat);

  char *ful = "/sys/class/power_supply/BAT0/energy_full";
  if ((bat = fopen(ful, "r")) == NULL) {
    perror("energy_full error");
    exit(1);
  }
  char all[8];
  fgets(all, 8, bat);
  fclose(bat);

  char **num_end = NULL;
  char **dem_end = NULL;

  float num = (float)strtol(now, num_end, 10);
  float dem = (float)strtol(all, dem_end, 10);

  if (retVal == '1'){
    return smprintf("+%0.f%c", ((num*100)/dem), 37);
  }else{
    return smprintf("-%0.f%c", ((num*100)/dem), 37);
  }
}

void settz(char *tzname){
  setenv("TZ", tzname, 1);
}

char *mktimes(char *fmt, char *tzname){
  char buf[129];
  time_t tim;
  struct tm *timtm;

  memset(buf, 0, sizeof(buf));
  settz(tzname);
  tim = time(NULL);
  timtm = localtime(&tim);
  if (timtm == NULL) {
    perror("localtime");
    exit(1);
  }

  if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
    fprintf(stderr, "strftime == 0\n");
    exit(1);
  }

  return smprintf("%s", buf);
}

char* getaddr(char *host){
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1){
    perror("getaddr");
    exit(1);
  }
  
  int family, s, n;
  host = calloc(NI_MAXHOST, sizeof(char));

  for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL) continue;
    if (strcmp(ifa->ifa_name, "lo") == 0) continue;

    family = ifa->ifa_addr->sa_family;
    
    if (family == AF_INET) {
      s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if(s != 0) {
        freeifaddrs(ifaddr);
        fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(s));
        exit(1);
      }
    }
  }
  freeifaddrs(ifaddr);
  return host;
}

void setstatus(char *str){
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

char *loadavg(void){
  double avgs[3];

  if (getloadavg(avgs, 3) < 0) {
    perror("getloadavg");
    exit(1);
  }

  return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

int main(void){
  char *status;
  char *avgs;
  char *tmusa;
  char *pow;
  char *host;

  if (!(dpy = XOpenDisplay(NULL))) {
    fprintf(stderr, "dwmstatus: cannot open display.\n");
    return 1;
  }

  for (;;sleep(90)) {
    avgs = loadavg();
    tmusa = mktimes("%a %d %b %H:%M %Z %Y", utcusa);
    pow = get_status();
    host = getaddr(host);
    status = smprintf("Load:%s | %s | %s | %s", avgs, host , pow, tmusa);
    setstatus(status);
    free(avgs);
    free(tmusa);
    free(status);
    free(host);
  }

  XCloseDisplay(dpy);

  return 0;
}


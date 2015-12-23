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

char get_ac_status(){
  char *ac_status = "/sys/class/power_supply/AC/online";
  char acVal;
  
  FILE *ac_power_fp = NULL;
  if ((ac_power_fp = fopen(ac_status, "r")) == NULL) {
    perror("fopen error: ac_power_fp");
  }
  acVal = fgetc(ac_power_fp);
  fclose(ac_power_fp);
  return acVal;
}

float get_num_status(){

  FILE *ba_power_fp = NULL;;
  char *ba_status = "/sys/class/power_supply/BAT0/energy_now";
  char current_power[10];
  char** num_end = NULL;
  
  if ((ba_power_fp = fopen(ba_status, "r")) == NULL) {
    perror("fopen error: ba_power_fp");
  }
  
  fgets(current_power, 10, ba_power_fp);
  fclose(ba_power_fp);
  
  return (float)strtol(current_power, num_end, 10);
}

float get_dem_status(){
  
  FILE *po_power_fp = NULL;;
  char *po_status = "/sys/class/power_supply/BAT0/energy_full";
  char total_power[10];
  char** dem_end = NULL;
  
  if ((po_power_fp = fopen(po_status, "r")) == NULL) {
    perror("fopen error: po_power_fp");
  }

  fgets(total_power, 10, po_power_fp);
  fclose(po_power_fp);
  
  return (float)strtol(total_power, dem_end, 10);
}

char *get_status() {
  char acVal = get_ac_status();
  float num = get_num_status();
  float dem = get_dem_status();

  if (acVal == '1'){
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
    perror("mktimes: localtime returned null");
  }

  if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
    perror("strftime == 0");
  }

  return smprintf("%s", buf);
}

char* getaddr(char *host){
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1){
    perror("getaddr: getifaddrs returned -1");
  }
  
  int family, s, n;
  host = calloc(NI_MAXHOST,sizeof(char));
  if (host == NULL){
    perror("getaddr: host set to NULL");
  }
  
  for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL) continue;
    if (strcmp(ifa->ifa_name, "lo") == 0) continue;

    family = ifa->ifa_addr->sa_family;
    
    if (family == AF_INET) {
      s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if(s != 0) {
        freeifaddrs(ifaddr);
        perror(smprintf("getnameinfo() failed: %s\n", gai_strerror(s)));
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
    free(status);
    free(avgs);
    free(tmusa);
    free(host);
    free(pow);
  }

  XCloseDisplay(dpy);

  return 0;
}


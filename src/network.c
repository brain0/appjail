#include "network.h"
#include "cap.h"
#include <net/if.h>
#include <netlink/route/addr.h>
#include <netlink/addr.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

int configure_loopback_interface() {
  struct nl_sock *sock = NULL;
  struct rtnl_addr *addr = NULL;
  struct nl_addr* lo_addr = NULL;
  struct nl_cache *cache = NULL;
  struct rtnl_link *link = NULL, *link2 = NULL;
  int err, nlflags = NLM_F_CREATE, ret = 0;
 
  if(!want_cap(CAP_NET_ADMIN)) {
    errWarn("Cannot set the CAP_NET_ADMIN effective capability");
    return -1;
  }
  
  sock = nl_socket_alloc();
  if(sock == NULL) {
    errWarn("nl_socket_alloc");
    return -1;
  }
  if((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
    fprintf(stderr, "Unable to connect to netlink: %s\n", nl_geterror(err));
    ret = -1;
    goto out2;
  }
  if(rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache) < 0) {
    ret = -1;
    goto out;
  }
  link = rtnl_link_get_by_name(cache, "lo");
  if (link == NULL) {
    ret = -1;
    goto out;
  }
  addr = rtnl_addr_alloc();
  if(addr == NULL) {
    ret = -1;
    goto out;
  }
 
  rtnl_addr_set_link(addr, link);
  rtnl_addr_set_family(addr, AF_INET);
  if((err = nl_addr_parse("127.0.0.1/8", AF_INET, &lo_addr)) < 0) {
    fprintf(stderr, "Unable to parse address: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }
  if((err = rtnl_addr_set_local(addr, lo_addr)) < 0) {
    fprintf(stderr, "Unable to set address: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }
  nl_addr_put(lo_addr);
  lo_addr = NULL;
  if ((err = rtnl_addr_add(sock, addr, nlflags)) < 0) {
    fprintf(stderr, "Unable to add address: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }

  rtnl_addr_set_family(addr, AF_INET6);
  if((err = nl_addr_parse("::1/128", AF_INET6, &lo_addr)) < 0) {
    fprintf(stderr, "Unable to parse address: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }
  if((err = rtnl_addr_set_local(addr, lo_addr)) < 0) {
    fprintf(stderr, "Unable to set address: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }
  nl_addr_put(lo_addr);
  lo_addr = NULL;
  if ((err = rtnl_addr_add(sock, addr, nlflags)) < 0) {
    fprintf(stderr, "Unable to add address: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }
  link2 = rtnl_link_alloc();
  if(link2 == NULL) {
    ret = -1;
    goto out;
  }
  rtnl_link_set_flags(link2, IFF_UP);
  if((err = rtnl_link_change(sock, link, link2, 0)) < 0) {
    fprintf(stderr, "Unable to change link: %s\n", nl_geterror(err));
    ret = -1;
    goto out;
  }

out:
  if(lo_addr!=NULL)
    nl_addr_put(lo_addr);
  if(link2!=NULL)  
    rtnl_link_put(link2);
  if(link!=NULL)  
    rtnl_link_put(link);
  if(cache!=NULL)
    nl_cache_put(cache);
  if(addr!=NULL)
    rtnl_addr_put(addr);
  nl_close(sock);
out2:
  nl_socket_free(sock);

  drop_caps();

  return ret;
}

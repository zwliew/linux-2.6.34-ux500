/*
 * Example program creating and configuring CAIF IP Interface
 * by use of IOCTLs.
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brandeland / sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/caif/if_caif.h>
#include <linux/caif/caif_socket.h>
#include <stdio.h>
#include <string.h>
main()
{
  int s;
  int r;

  /* Set CAIF IP Inteface config parameters */
  static struct ifcaif_param param = {
    .ipv4_connid = 1,
    .loop = 1

  };

  /* Populate ifreq parameters */
  static struct ifreq ifr = {
    .ifr_name = "caifioc%d",
    .ifr_ifru.ifru_data = (void *) &param

  };

  /* Create a CAIF socket */
  s = socket(AF_CAIF, SOCK_SEQPACKET, CAIFPROTO_AT);

  /* Create new IP Interface */
  r = ioctl(s, SIOCCAIFNETNEW, &ifr);
  printf("Result=%d\n", r);

  /* General Interface IOC are available, e.g. find ifindex */
  strcpy(ifr.ifr_name, "caifioc0");
  r = ioctl(s, SIOCGIFINDEX, &ifr);
  printf("Result=%d\n", r);


  r = ioctl(s, 	SIOCCAIFNETREMOVE, &ifr);
  printf("Result=%d\n", r);

  printf("res=%d, ifindex=%d\n", r, ifr.ifr_ifindex);
}

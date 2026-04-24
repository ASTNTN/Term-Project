#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/genl/genl.h>
#include <libnl3/netlink/genl/ctrl.h>
#include <linux/nl80211.h>

static int station_handler(struct nl_msg *msg, void *arg) {
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

    nla_parse(tb, NL80211_ATTR_MAX,
              genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (tb[NL80211_ATTR_STA_INFO]) {
        struct nlattr *sta_info[NL80211_STA_INFO_MAX + 1];

        nla_parse_nested(sta_info, NL80211_STA_INFO_MAX,
                         tb[NL80211_ATTR_STA_INFO], NULL);

        if (sta_info[NL80211_STA_INFO_SIGNAL]) {
            int8_t rssi = nla_get_u8(sta_info[NL80211_STA_INFO_SIGNAL]);
            printf("RSSI: %d dBm\n", rssi);
        }
    }

    return NL_SKIP;
}
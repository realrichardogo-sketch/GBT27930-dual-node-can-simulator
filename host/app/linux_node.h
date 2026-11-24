/* app/linux_node.h - SocketCAN node process runner (Stage 6B). */
#ifndef GBT_LINUX_NODE_H
#define GBT_LINUX_NODE_H

#include <stdint.h>

/* role: GBT_NODE_ROLE_CHARGER or GBT_NODE_ROLE_VEHICLE. Returns the process exit code:
 * 0 expected final phase reached, 1 not reached / failure, 2 SocketCAN unavailable, 3 usage. */
int gbt_linux_node_main(uint8_t role, int argc, char **argv);

#endif /* GBT_LINUX_NODE_H */

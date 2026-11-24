/* app/linux_charger_main.c - charger node process (SocketCAN, Stage 6B). */
#include "app/linux_node.h"
#include "protocol/gbt_node.h"

int main(int argc, char **argv)
{
    return gbt_linux_node_main((uint8_t)GBT_NODE_ROLE_CHARGER, argc, argv);
}

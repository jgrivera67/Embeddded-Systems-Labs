/**
 * @file networking.c
 *
 * Networking stack main module
 *
 * @author German Rivera
 */
#include "networking.h"
#include "networking_layer2.h"
#include "networking_layer3.h"
#include "networking_layer4.h"

/**
 * Global initialization of the networking stack
 */
void networking_init(void)
{
    net_layer2_init();
    net_layer3_init();
    net_layer4_init();
    net_layer2_start();
    net_layer3_start_tasks();
}

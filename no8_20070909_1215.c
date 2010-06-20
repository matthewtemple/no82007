/*  no8.c - created 2007 by inhaesio zha (zha@inhesion.com)                  */
/*  gcc -ansi -O3 -L/usr/X11R6/lib -lX11 -o no8 no8.c                        */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define SEED 1
#define NODES 256
#define TIME 256

/*  add an action that swaps two nodes (position, values, everything)?       */
#define ACTION_2_DO_NOTHING 0
#define ACTION_2_BECOME_0 1
#define ACTION_2_BECOME_1 2
#define ACTION_2_LINK_0_BECOME_0 3
#define ACTION_2_LINK_0_BECOME_1 4
#define ACTION_2_LINK_1_BECOME_0 5
#define ACTION_2_LINK_1_BECOME_1 6
#define ACTION_2_CONNECT_LINKS_0_AND_1 7
#define ACTION_2_SWAP_VALUE_WITH_LINK_0 8
#define ACTION_2_SWAP_VALUE_WITH_LINK_1 9
#define ACTION_2_COUNT 10

#define ACTION_3_DO_NOTHING 0
#define ACTION_3_DISCONNECT_LINK_0 1
#define ACTION_3_DISCONNECT_LINK_1 2
#define ACTION_3_DISCONNECT_LINK_2 3
#define ACTION_3_BECOME_0 4
#define ACTION_3_BECOME_1 5
#define ACTION_3_LINK_0_BECOME_0 6
#define ACTION_3_LINK_0_BECOME_1 7
#define ACTION_3_LINK_1_BECOME_0 8
#define ACTION_3_LINK_1_BECOME_1 9
#define ACTION_3_LINK_2_BECOME_0 10
#define ACTION_3_LINK_2_BECOME_1 11
#define ACTION_3_CONNECT_LINKS_0_AND_1 12
#define ACTION_3_CONNECT_LINKS_1_AND_2 13
#define ACTION_3_CONNECT_LINKS_2_AND_0 14
#define ACTION_3_SWAP_VALUE_WITH_LINK_0 15
#define ACTION_3_SWAP_VALUE_WITH_LINK_1 16
#define ACTION_3_SWAP_VALUE_WITH_LINK_2 17
#define ACTION_3_COUNT 18

#define ACTION_TABLE_2_SIZE 2 * 2 * 2
#define ACTION_TABLE_3_SIZE 2 * 2 * 2 * 2

#define SLEEP 1024

struct node_t {
	unsigned int value;
	unsigned int link_count;
	struct node_t *link_0;
	struct node_t *link_1;
	struct node_t *link_2;
};
typedef struct node_t node_t;

struct network_t {
	node_t *nodes[NODES];
};
typedef struct network_t network_t;

static void connect_nodes(node_t *node_a, node_t *node_b);
static network_t *create_network();
static void destroy_network(network_t *network);
static void disconnect_nodes(node_t *node_a, node_t *node_b);
/*  static int get_action(node_t *node);  */
static int get_action_2(node_t *node);
static int get_action_3(node_t *node);
static void init_action_tables();
static void iterate_network(network_t *network);
static void iterate_node(node_t *node);
static void iterate_node_2(node_t *node);
static void iterate_node_3(node_t *node);
static unsigned int node_can_connect(node_t *node);
static unsigned int node_can_disconnect(node_t *node);
static void shuffle_down_links(node_t *node);
static void swap_values(node_t *node_a, node_t *node_b);

int action_table_2[ACTION_TABLE_2_SIZE];
int action_table_3[ACTION_TABLE_3_SIZE];

void connect_nodes(node_t *node_a, node_t *node_b)
{
	if (node_can_connect(node_a) && node_can_connect(node_b)) {
		node_a->link_2 = node_b;
		node_b->link_2 = node_a;
		node_a->link_count = 3;
		node_b->link_count = 3;
	}
}

network_t *create_network()
{
	network_t *network;
	unsigned int each_node;
	node_t *node;

	network = malloc(sizeof(network_t));

	for (each_node = 0; each_node < NODES; each_node++) {
		node = malloc(sizeof(node_t));
		node->value = rand() % 2;
		node->link_count = 2;
		node->link_2 = 0;
		network->nodes[each_node] = node;
	}

	for (each_node = 1; each_node < (NODES - 1); each_node++) {
		node = network->nodes[each_node];
		node->link_0 = network->nodes[each_node - 1];
		node->link_1 = network->nodes[each_node + 1];
	}

	node = network->nodes[0];
	node->link_0 = network->nodes[NODES - 1];
	node->link_1 = network->nodes[1];

	node = network->nodes[NODES - 1];
	node->link_0 = network->nodes[NODES - 2];
	node->link_1 = network->nodes[0];

	return network;
}

void destroy_network(network_t *network)
{
	unsigned int each_node;

	for (each_node = 0; each_node < NODES; each_node++) {
		free(network->nodes[each_node]);
	}
}

void disconnect_nodes(node_t *node_a, node_t *node_b)
{
	if (node_can_disconnect(node_a) && node_can_disconnect(node_b)) {

		if (node_b == node_a->link_0) {
			node_a->link_0 = 0;
			shuffle_down_links(node_a);
		}
		else if (node_b == node_a->link_1) {
			node_a->link_1 = 0;
			shuffle_down_links(node_a);
		}
		else if (node_b == node_a->link_2) {
			node_a->link_2 = 0;
		}
		else {
			exit(23);
		}

		if (node_a == node_b->link_0) {
			node_b->link_0 = 0;
			shuffle_down_links(node_b);
		}
		else if (node_a == node_b->link_1) {
			node_b->link_1 = 0;
			shuffle_down_links(node_b);
		}
		else if (node_a == node_b->link_2) {
			node_b->link_2 = 0;
		}
		else {
			exit(24);
		}

		node_a->link_count = 2;
		node_b->link_count = 2;
	}
}

/*
int get_action(node_t *node)
{
	int action;

	if (2 == node->link_count) {
		action = get_action_2(node);
	}
	else {
		action = get_action_3(node);
	}

	return action;
}
*/

int get_action_2(node_t *node)
{
	int action;
	unsigned int action_index;

	action_index = (1 * node->value) + (2 * node->link_0->value)
		+ (4 * node->link_1->value);
	action = action_table_2[action_index];

	return action;
}

int get_action_3(node_t *node)
{
	int action;
	unsigned int action_index;

	action_index = (1 * node->value) + (2 * node->link_0->value)
		+ (4 * node->link_1->value) + (8 * node->link_2->value);
	action = action_table_3[action_index];

	return action;
}

void init_action_tables()
{
	unsigned int each_rule;

	for (each_rule = 0; each_rule < ACTION_TABLE_2_SIZE; each_rule++) {
		action_table_2[each_rule] = rand() % ACTION_2_COUNT;
	}

	for (each_rule = 0; each_rule < ACTION_TABLE_3_SIZE; each_rule++) {
		action_table_3[each_rule] = rand() % ACTION_3_COUNT;
	}
}

void iterate_network(network_t *network)
{
	unsigned int each_node;

	for (each_node = 0; each_node < NODES; each_node++) {
		iterate_node(network->nodes[each_node]);
	}
}

void iterate_node(node_t *node)
{
	if (2 == node->link_count) {
		iterate_node_2(node);
	}
	else {
		iterate_node_3(node);
	}
}

void iterate_node_2(node_t *node)
{
	int action;

	action = get_action_2(node);

	switch (action) {
		case ACTION_2_DO_NOTHING:
			break;
		case ACTION_2_BECOME_0:
			node->value = 0;
			break;
		case ACTION_2_BECOME_1:
			node->value = 1;
			break;
		case ACTION_2_LINK_0_BECOME_0:
			node->link_0->value = 0;
			break;
		case ACTION_2_LINK_0_BECOME_1:
			node->link_0->value = 1;
			break;
		case ACTION_2_LINK_1_BECOME_0:
			node->link_1->value = 0;
			break;
		case ACTION_2_LINK_1_BECOME_1:
			node->link_1->value = 1;
			break;
		case ACTION_2_CONNECT_LINKS_0_AND_1:
			connect_nodes(node->link_0, node->link_1);
			break;
		case ACTION_2_SWAP_VALUE_WITH_LINK_0:
			swap_values(node, node->link_0);
			break;
		case ACTION_2_SWAP_VALUE_WITH_LINK_1:
			swap_values(node, node->link_1);
			break;
		default:
			exit(25);
	}
}

void iterate_node_3(node_t *node)
{
	int action;

	action = get_action_3(node);

	switch (action) {
		case ACTION_3_DO_NOTHING:
			break;
		case ACTION_3_DISCONNECT_LINK_0:
			disconnect_nodes(node, node->link_0);
			break;
		case ACTION_3_DISCONNECT_LINK_1:
			disconnect_nodes(node, node->link_1);
			break;
		case ACTION_3_DISCONNECT_LINK_2:
			disconnect_nodes(node, node->link_2);
			break;
		case ACTION_3_BECOME_0:
			node->value = 0;
			break;
		case ACTION_3_BECOME_1:
			node->value = 1;
			break;
		case ACTION_3_LINK_0_BECOME_0:
			node->link_0->value = 0;
			break;
		case ACTION_3_LINK_0_BECOME_1:
			node->link_0->value = 1;
			break;
		case ACTION_3_LINK_1_BECOME_0:
			node->link_1->value = 0;
			break;
		case ACTION_3_LINK_1_BECOME_1:
			node->link_1->value = 1;
			break;
		case ACTION_3_LINK_2_BECOME_0:
			node->link_2->value = 0;
			break;
		case ACTION_3_LINK_2_BECOME_1:
			node->link_2->value = 1;
			break;
		case ACTION_3_CONNECT_LINKS_0_AND_1:
			connect_nodes(node->link_0, node->link_1);
			break;
		case ACTION_3_CONNECT_LINKS_1_AND_2:
			connect_nodes(node->link_1, node->link_2);
			break;
		case ACTION_3_CONNECT_LINKS_2_AND_0:
			connect_nodes(node->link_2, node->link_0);
			break;
		case ACTION_3_SWAP_VALUE_WITH_LINK_0:
			swap_values(node, node->link_0);
			break;
		case ACTION_3_SWAP_VALUE_WITH_LINK_1:
			swap_values(node, node->link_1);
			break;
		case ACTION_3_SWAP_VALUE_WITH_LINK_2:
			swap_values(node, node->link_2);
			break;
		default:
			exit(26);
	}
}

unsigned int node_can_connect(node_t *node)
{
	unsigned int can_connect;

	if (2 == node->link_count) {
		can_connect = 1;
	}
	else {
		can_connect = 0;
	}

	return can_connect;
}

unsigned int node_can_disconnect(node_t *node)
{
	unsigned int can_disconnect;

	if (3 == node->link_count) {
		can_disconnect = 1;
	}
	else {
		can_disconnect = 0;
	}

	return can_disconnect;
}

void shuffle_down_links(node_t *node)
{
	if (!node->link_0) {
		node->link_0 = node->link_1;
		node->link_1 = node->link_2;
		node->link_2 = 0;
	}
	else if (!node->link_1) {
		node->link_1 = node->link_2;
		node->link_2 = 0;
	}
}

void swap_values(node_t *node_a, node_t *node_b)
{
	unsigned int temporary_value;

	temporary_value = node_a->value;
	node_a->value = node_b->value;
	node_b->value = temporary_value;
}

int main(int argc, char *argv[])
{
	network_t *network;
	unsigned int moment;
	unsigned int node_index;
	node_t *node;

	Display *display;
	GC gc;
	int screen_number;
	int window_x;
	int window_y;
	unsigned int each_x;
	unsigned int each_y;
	unsigned int window_border_width;
	unsigned int window_height;
	unsigned int window_width;
	unsigned long gc_value_mask;
	unsigned long window_background_color;
	unsigned long window_border_color;
	Window root_window;
	Window window;
	XGCValues gc_values;
	Visual* default_visual;
	Colormap colormap;
	XColor system_color;
	XColor exact_color;

	display = XOpenDisplay(NULL);
	screen_number = DefaultScreen(display);
	root_window = RootWindow(display, screen_number);
	window_x = 0;
	window_y = 0;
	window_width = NODES;
	window_height = TIME;
	window_border_width = 0;
	window_border_color = BlackPixel(display, screen_number);
	window_background_color = WhitePixel(display, screen_number);
	window = XCreateSimpleWindow(display, root_window, window_x, window_y,
			window_width, window_height, window_border_width,
			window_border_color, window_background_color);
	XMapWindow(display, window);
	XFlush(display);
	gc_value_mask = 0;
	gc = XCreateGC(display, window, gc_value_mask, &gc_values);
	default_visual = DefaultVisual(display, DefaultScreen(display));
	colormap = XCreateColormap(display, window, default_visual, AllocNone);
	XSync(display, False);

	srandom(SEED);
	init_action_tables();
	network = create_network();

	for (moment = 0; moment < TIME; moment++) {
		for (node_index = 0; node_index < NODES; node_index++) {
			node = network->nodes[node_index];

			system_color.red = 0;
			if (2 == node->link_count) {
				system_color.green = USHRT_MAX / 2;
			}
			else if (3 == node->link_count) {
				system_color.green = USHRT_MAX;
			}
			else {
				exit(99);
			}

			/*
			if (0 == node->value) {
				system_color.blue = USHRT_MAX / 2;
			}
			else if (1 == node->value) {
				system_color.blue = USHRT_MAX;
			}
			else {
				exit(100);
			}
			*/
			system_color.blue = 0;

			XAllocColor(display, colormap, &system_color);
			XSetForeground(display, gc, system_color.pixel);

			XDrawPoint(display, window, gc, node_index, moment);
		}
		XFlush(display);

		iterate_network(network);
	}

	while (1) {
		usleep(SLEEP);
	}

	destroy_network(network);

	XUnmapWindow(display, window);
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}

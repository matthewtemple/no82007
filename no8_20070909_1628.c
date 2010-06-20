/*  no8.c - created 2007 by inhaesio zha (zha@inhesion.com)                  */
/*  gcc -ansi -O3 -L/usr/X11R6/lib -lX11 -o no8 no8.c                        */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define SEED 0
#define NODES 1024
#define TIME 768

#define XWINDOW_WIDTH 1024

/*
#define ACTION_2_SWAP_WITH_LINK_0 0
#define ACTION_2_SWAP_WITH_LINK_1 1
#define ACTION_2_SWAP_VALUE_WITH_LINK_0 2
#define ACTION_2_SWAP_VALUE_WITH_LINK_1 3
#define ACTION_2_CONNECT_LINKS_0_AND_1 4
#define ACTION_2_INVERT_VALUE 5
#define ACTION_2_LINK_0_BECOME_MY_VALUE 6
#define ACTION_2_LINK_0_BECOME_0 7
#define ACTION_2_LINK_0_BECOME_1 8
#define ACTION_2_LINK_0_BECOME_MY_INVERTED_VALUE 9
#define ACTION_2_LINK_1_BECOME_MY_VALUE 10
#define ACTION_2_LINK_1_BECOME_0 11
#define ACTION_2_LINK_1_BECOME_1 12
#define ACTION_2_LINK_1_BECOME_MY_INVERTED_VALUE 13
#define ACTION_2_DO_NOTHING 14
#define ACTION_2_BECOME_0 15
#define ACTION_2_BECOME_1 16
#define ACTION_2_COUNT 17

#define ACTION_3_SWAP_WITH_LINK_0 0
#define ACTION_3_SWAP_WITH_LINK_1 1
#define ACTION_3_SWAP_WITH_LINK_2 2
#define ACTION_3_SWAP_VALUE_WITH_LINK_0 3
#define ACTION_3_SWAP_VALUE_WITH_LINK_1 4
#define ACTION_3_SWAP_VALUE_WITH_LINK_2 5
#define ACTION_3_CONNECT_LINKS_0_AND_1 6
#define ACTION_3_CONNECT_LINKS_1_AND_2 7
#define ACTION_3_CONNECT_LINKS_2_AND_0 8
#define ACTION_3_DISCONNECT_LINK_0 9
#define ACTION_3_DISCONNECT_LINK_1 10
#define ACTION_3_DISCONNECT_LINK_2 11
#define ACTION_3_INVERT_VALUE 12
#define ACTION_3_LINK_0_BECOME_MY_VALUE 13
#define ACTION_3_LINK_0_BECOME_0 14
#define ACTION_3_LINK_0_BECOME_1 15
#define ACTION_3_LINK_0_BECOME_MY_INVERTED_VALUE 16
#define ACTION_3_LINK_1_BECOME_MY_VALUE 17
#define ACTION_3_LINK_1_BECOME_0 18
#define ACTION_3_LINK_1_BECOME_1 19
#define ACTION_3_LINK_1_BECOME_MY_INVERTED_VALUE 20
#define ACTION_3_LINK_2_BECOME_MY_VALUE 21
#define ACTION_3_LINK_2_BECOME_0 22
#define ACTION_3_LINK_2_BECOME_1 23
#define ACTION_3_LINK_2_BECOME_MY_INVERTED_VALUE 24
#define ACTION_3_DO_NOTHING 25
#define ACTION_3_BECOME_0 26
#define ACTION_3_BECOME_1 27
#define ACTION_3_COUNT 28
*/

/*
#define ACTION_TABLE_2_SIZE 2 * 2 * 2
#define ACTION_TABLE_3_SIZE 2 * 2 * 2 * 2
*/

#define SLEEP 1024

struct node_t {
	unsigned int value;
	unsigned int link_count;
	struct node_t *link_0;
	struct node_t *link_1;
	struct node_t *link_2;
	unsigned int display_index;
};
typedef struct node_t node_t;

struct network_t {
	node_t *nodes[NODES];
	node_t *nodes_display_order[NODES];
};
typedef struct network_t network_t;

static void connect_nodes(node_t *this_node, node_t *node_a, node_t *node_b);
static network_t *create_network();
static void destroy_network(network_t *network);
static void disconnect_nodes(node_t *node_a, node_t *node_b);
static int get_action_2(node_t *node);
static int get_action_3(node_t *node);
/*  static void init_action_tables();  */
static void invert_value(node_t *node);
static unsigned int inverted_value(node_t *node);
static void iterate_network(network_t *network);
static void iterate_node(node_t *node);
static void iterate_node_2(node_t *node);
static void iterate_node_3(node_t *node);
static node_t *link_not_me(node_t *node_to_search, node_t *me);
static unsigned int node_can_connect(node_t *node);
static unsigned int node_can_disconnect(node_t *node);
static unsigned int node_is_connected_to(node_t *node_from, node_t *node_to);
static void nothing();
static void redirect_link(node_t *node_to_change, node_t *old_link,
		node_t *new_link);
static void shuffle_down_links(node_t *node);
static void swap_nodes(node_t *node_a, node_t *node_b);
static void swap_values(node_t *node_a, node_t *node_b);

/*
int action_table_2[ACTION_TABLE_2_SIZE];
int action_table_3[ACTION_TABLE_3_SIZE];
*/

network_t *network;

void connect_nodes(node_t *this_node, node_t *node_a, node_t *node_b)
{
	node_t *node_a_link_not_me;
	node_t *node_b_link_not_me;

	if (!node_a || !node_b) {
		return;
	}

	/*  TODO: take a deeper look at why this condition happens               */
	if ((this_node == node_a) || (this_node == node_b) || (node_a == node_b)) {
		/*  nothing();  */
		return;
	}

	if (node_can_connect(node_a)) {
		if (!node_is_connected_to(node_a, node_b)) {
			node_a->link_2 = node_b;
			node_a->link_count = 3;
		}
	}
	else if (node_can_connect(node_b)) {
		if (!node_is_connected_to(node_b, node_a)) {
			node_b->link_2 = node_a;
			node_b->link_count = 3;
		}
	}
	else {
		node_a_link_not_me = link_not_me(node_a, this_node);
		if (node_a_link_not_me) {
			redirect_link(node_a, node_a_link_not_me, node_b);
			redirect_link(this_node, node_a, node_a_link_not_me);
		}

		node_b_link_not_me = link_not_me(node_b, this_node);
		if (node_b_link_not_me) {
			redirect_link(node_b, node_b_link_not_me, node_a);
			redirect_link(this_node, node_b, node_b_link_not_me);
		}
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
		node->display_index = each_node;
		network->nodes[each_node] = node;
		network->nodes_display_order[each_node] = node;
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
	if (node_a) {
		if (node_can_disconnect(node_a)) {
			if (node_b == node_a->link_0) {
				node_a->link_0 = 0;
				shuffle_down_links(node_a);
			}
			else if (node_b == node_a->link_1) {
				node_a->link_1 = 0;
				shuffle_down_links(node_a);
			}
			else if (node_a->link_2) {
				if (node_b == node_a->link_2) {
					node_a->link_2 = 0;
				}
			}
			else {
				exit(23);
			}
			node_a->link_count = 2;
		}
	}

	if (node_b) {
		if (node_can_disconnect(node_b)) {
			if (node_a == node_b->link_0) {
				node_b->link_0 = 0;
				shuffle_down_links(node_b);
			}
			else if (node_a == node_b->link_1) {
				node_b->link_1 = 0;
				shuffle_down_links(node_b);
			}
			else if (node_b->link_2) {
				if (node_a == node_b->link_2) {
					node_b->link_2 = 0;
				}
			}
			else {
				exit(24);
			}
			node_b->link_count = 2;
		}
	}
}

/*
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
		else if (node_a->link_2) {
			if (node_b == node_a->link_2) {
				node_a->link_2 = 0;
			}
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
		else if (node_b->link_2) {
			if (node_a == node_b->link_2) {
				node_b->link_2 = 0;
			}
		}
		else {
			exit(24);
		}

		node_a->link_count = 2;
		node_b->link_count = 2;
	}
}
*/

int get_action_2(node_t *node)
{
	int action;
	unsigned int action_index;

	action_index = (1 * node->value) + (2 * node->link_0->value)
		+ (4 * node->link_1->value);
	/*  action = action_table_2[action_index];  */

	/*  return action;  */
	return action_index;
}

int get_action_3(node_t *node)
{
	int action;
	unsigned int action_index;

	action_index = (1 * node->value) + (2 * node->link_0->value)
		+ (4 * node->link_1->value) + (8 * node->link_2->value);
	/*  action = action_table_3[action_index];  */

	/*  return action;  */
	return action_index;
}

/*
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
*/

void invert_value(node_t *node)
{
	node->value = inverted_value(node);
}

unsigned int inverted_value(node_t *node)
{
	unsigned int inverted_value;

	if (0 == node->value) {
		inverted_value = 1;
	}
	else {
		inverted_value = 0;
	}

	return inverted_value;
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

		case 0:
			connect_nodes(node, node->link_0, node->link_1);
			break;
		case 6:
			swap_values(node, node->link_0);
			break;
		case 1:
		case 7:
			swap_values(node, node->link_1);
			break;

		case 2:
		case 4:
			swap_nodes(node, node->link_0);
			break;
		case 3:
		case 5:
			swap_nodes(node, node->link_1);
			break;
	}
	/*
	  node->value = 0;
	  node->value = 1;
	  invert_value(node);
	  connect_nodes(node, node->link_0, node->link_1);
	  swap_values(node, node->link_0);
	  swap_values(node, node->link_1);
	  node->link_0->value = node->value;
	  node->link_0->value = 0;
	  node->link_0->value = 1;
	  node->link_0->value = inverted_value(node);
	  node->link_1->value = node->value;
	  node->link_1->value = 0;
	  node->link_1->value = 1;
	  node->link_1->value = inverted_value(node);
	  swap_nodes(node, node->link_0);
	  swap_nodes(node, node->link_1);
	*/
}

void iterate_node_3(node_t *node)
{
	int action;

	action = get_action_3(node);

	switch (action) {
		case 0:
		case 9:
			connect_nodes(node, node->link_0, node->link_1);
			break;
		case 1:
		case 10:
			connect_nodes(node, node->link_1, node->link_2);
			break;
		case 2:
		case 11:
			connect_nodes(node, node->link_2, node->link_0);
			break;

		case 3:
		case 12:
			disconnect_nodes(node, node->link_0);
			break;
		case 4:
		case 13:
			disconnect_nodes(node, node->link_1);
			break;
		case 5:
		case 14:
			disconnect_nodes(node, node->link_2);
			break;

		case 6:
		case 15:
			swap_values(node, node->link_0);
			break;
		case 7:
			swap_values(node, node->link_1);
			break;
		case 8:
			swap_values(node, node->link_2);
			break;
	}
	/*
	  node->value = 0;
	  node->value = 1;
	  invert_value(node);
	  disconnect_nodes(node, node->link_0);
	  disconnect_nodes(node, node->link_1);
	  disconnect_nodes(node, node->link_2);
	  connect_nodes(node, node->link_0, node->link_1);
	  connect_nodes(node, node->link_1, node->link_2);
	  connect_nodes(node, node->link_2, node->link_0);
	  swap_values(node, node->link_0);
	  swap_values(node, node->link_1);
	  swap_values(node, node->link_2);
	  node->link_0->value = node->value;
	  node->link_0->value = 0;
	  node->link_0->value = 1;
	  node->link_0->value = inverted_value(node);
	  node->link_1->value = node->value;
	  node->link_1->value = 0;
	  node->link_1->value = 1;
	  node->link_1->value = inverted_value(node);
	  node->link_2->value = node->value;
	  node->link_2->value = 0;
	  node->link_2->value = 1;
	  node->link_2->value = inverted_value(node);
	  swap_nodes(node, node->link_0);
	  swap_nodes(node, node->link_1);
	  swap_nodes(node, node->link_2);
	*/
}

node_t *link_not_me(node_t *node_to_search, node_t *me)
{
	node_t *link = 0;

	if (!node_is_connected_to(node_to_search, me)) {
		if (node_to_search->link_0 != me) {
			link = node_to_search->link_0;
		}
		else if (node_to_search->link_1 != me) {
			link = node_to_search->link_1;
		}
		else if (node_to_search->link_2) {
			if (node_to_search->link_2 != me) {
				link = node_to_search->link_2;
			}
		}

		/*  this shouldn't happen anymore                                    */
		if (!link) {
			/*  nothing();  */
			exit(13);
		}
	}

	return link;
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

unsigned int node_is_connected_to(node_t *node_from, node_t *node_to)
{
	unsigned int connected;

	if ((node_from->link_0 == node_to) || (node_from->link_1 == node_to)
			|| (node_from->link_2 == node_to)) {
		connected = 1;
	}
	else {
		connected = 0;
	}

	return connected;
}

void nothing()
{
	return;
}

void redirect_link(node_t *node_to_change, node_t *old_link, node_t *new_link)
{
	if (node_to_change != new_link) {
		if (node_to_change->link_0 == old_link) {
			node_to_change->link_0 = new_link;
		}
		else if (node_to_change->link_1 == old_link) {
			node_to_change->link_1 = new_link;
		}
		else if (node_to_change->link_2 == old_link) {
			node_to_change->link_2 = new_link;
		}
	}
}

void shuffle_down_links(node_t *node)
{
	if (!node->link_0) {
		node->link_0 = node->link_2;
		node->link_2 = 0;
	}
	else if (!node->link_1) {
		node->link_1 = node->link_2;
		node->link_2 = 0;
	}
}

void swap_nodes(node_t *node_a, node_t *node_b)
{
	unsigned int temporary_display_index;

	redirect_link(node_a->link_0, node_a, node_b);
	redirect_link(node_a->link_1, node_a, node_b);
	if (node_a->link_2) {
		redirect_link(node_a->link_2, node_a, node_b);
	}

	redirect_link(node_b->link_0, node_b, node_a);
	redirect_link(node_b->link_1, node_b, node_a);
	if (node_b->link_2) {
		redirect_link(node_b->link_2, node_b, node_a);
	}

	network->nodes_display_order[node_a->display_index] = node_b;
	network->nodes_display_order[node_b->display_index] = node_a;

	temporary_display_index = node_a->display_index;
	node_a->display_index = node_b->display_index;
	node_b->display_index = temporary_display_index;
}

void swap_values(node_t *node_a, node_t *node_b)
{
	unsigned int temporary_value;

	if (!node_a || !node_b) {
		return;
	}

	temporary_value = node_a->value;
	node_a->value = node_b->value;
	node_b->value = temporary_value;
}

int main(int argc, char *argv[])
{
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
	unsigned int window_display_modulus;
	unsigned int window_x_pixel;

	display = XOpenDisplay(NULL);
	screen_number = DefaultScreen(display);
	root_window = RootWindow(display, screen_number);
	window_x = 0;
	window_y = 0;
	window_width = XWINDOW_WIDTH;
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
	window_display_modulus = NODES / XWINDOW_WIDTH;

	srandom(SEED);
	/*  init_action_tables();  */
	network = create_network();

	for (moment = 0; moment < TIME; moment++) {
		window_x_pixel = 0;
		for (node_index = 0; node_index < NODES; node_index++) {
			if (0 != (node_index % window_display_modulus)) {
				continue;
			}

			node = network->nodes_display_order[node_index];

			system_color.red = 0;

			if (2 == node->link_count) {
				system_color.green = USHRT_MAX / 4;
			}
			else if (3 == node->link_count) {
				system_color.green = USHRT_MAX;
			}
			else {
				exit(99);
			}
			/*
			system_color.green = 0;
			*/

			if (0 == node->value) {
				system_color.blue = USHRT_MAX / 4;
			}
			else if (1 == node->value) {
				system_color.blue = USHRT_MAX;
			}
			else {
				exit(100);
			}
			/*
			system_color.blue = 0;
			*/

			XAllocColor(display, colormap, &system_color);
			XSetForeground(display, gc, system_color.pixel);

			XDrawPoint(display, window, gc, window_x_pixel, moment);
			window_x_pixel++;
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

/*  no8.c - created 2007 by inhaesio zha (zha@inhesion.com)                  */
/*  gcc -ansi -O3 -L/usr/X11R6/lib -lX11 -o no8 no8.c                        */

/*  + use graphviz to generate graphs of network connectivity through time   */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define SEED 4
#define NODES 128
/*  TODO: there's a range-oriented bug that's tickled if you set this too
	high                                                                     */
#define TIME 768

#define XWINDOW_WIDTH NODES
#define XWINDOW_HEIGHT TIME

#define SLEEP 1024

#define MAX_CONNECTIVITY 3 * NODES

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
		node->value = random() % 2;
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

int get_action_2(node_t *node)
{
	int action;
	unsigned int action_index;

	action_index = (1 * node->value) + (2 * node->link_0->value)
		+ (4 * node->link_1->value);

	return action_index;
}

int get_action_3(node_t *node)
{
	int action;
	unsigned int action_index;

	action_index = (1 * node->value) + (2 * node->link_0->value)
		+ (4 * node->link_1->value) + (8 * node->link_2->value);

	return action_index;
}

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

	action = get_action_2(node) % 4;

	switch (action) {
		case 0:
			swap_nodes(node, node->link_0);
			connect_nodes(node, node->link_0, node->link_1);
			break;
		case 1:
			swap_nodes(node, node->link_1);
			connect_nodes(node, node->link_1, node->link_0);
			break;
		case 2:
			swap_nodes(node, node->link_0);
			connect_nodes(node, node->link_1, node->link_0);
			break;
		case 3:
			swap_nodes(node, node->link_1);
			connect_nodes(node, node->link_0, node->link_1);
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

	action = get_action_3(node) % 6;

	switch (action) {
		case 0:
			swap_nodes(node, node->link_0);
			connect_nodes(node, node->link_0, node->link_1);
			invert_value(node);
			break;
		case 1:
			swap_nodes(node, node->link_1);
			connect_nodes(node, node->link_1, node->link_2);
			invert_value(node);
			break;
		case 2:
			swap_nodes(node, node->link_2);
			connect_nodes(node, node->link_2, node->link_0);
			invert_value(node);
			break;
		case 3:
			disconnect_nodes(node, node->link_0);
			connect_nodes(node, node->link_0, node->link_1);
			break;
		case 4:
			disconnect_nodes(node, node->link_1);
			connect_nodes(node, node->link_1, node->link_2);
			break;
		case 5:
			disconnect_nodes(node, node->link_2);
			connect_nodes(node, node->link_2, node->link_0);
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

	if (node_a && node_b) {
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
	unsigned int moment_index;
	node_t *node;
	unsigned int total_connectivity;

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
	unsigned int window_display_x_modulus;
	unsigned int window_display_y_modulus;
	unsigned int window_x_pixel;
	unsigned int window_y_pixel;

	display = XOpenDisplay(NULL);
	screen_number = DefaultScreen(display);
	root_window = RootWindow(display, screen_number);
	window_x = 0;
	window_y = 0;
	window_width = XWINDOW_WIDTH;
	window_height = XWINDOW_HEIGHT;
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
	window_display_x_modulus = NODES / XWINDOW_WIDTH;
	window_display_y_modulus = TIME / XWINDOW_HEIGHT;

	srandom(SEED);
	network = create_network();

	window_y_pixel = 0;
	for (moment = 0; moment < TIME; moment++) {
		total_connectivity = 0;
		if (0 == (moment % window_display_y_modulus)) {
			for (node_index = 0; node_index < NODES; node_index++) {
				node = network->nodes_display_order[node_index];
				total_connectivity += node->link_count;
			}
			window_x_pixel = 0;
			for (node_index = 0; node_index < NODES; node_index++) {
				if (0 != (node_index % window_display_x_modulus)) {
					continue;
				}

				node = network->nodes_display_order[node_index];

				system_color.red = (total_connectivity / MAX_CONNECTIVITY)
					% (USHRT_MAX / 2);

				if (0 == node->value) {
					system_color.green = USHRT_MAX / 4;
					system_color.blue = USHRT_MAX;
				}
				else if (1 == node->value) {
					system_color.green = USHRT_MAX;
					system_color.blue = USHRT_MAX / 4;
				}
				else {
					exit(100);
				}

				if (2 == node->link_count) {
					system_color.green /= 2;
					system_color.blue /= 2;
				}

				XAllocColor(display, colormap, &system_color);
				XSetForeground(display, gc, system_color.pixel);

				XDrawPoint(display, window, gc, window_x_pixel,
						window_y_pixel);
				window_x_pixel++;
			}
			XFlush(display);
			window_y_pixel++;
		}
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

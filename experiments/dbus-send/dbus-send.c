#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

static void append_arg (DBusMessageIter *iter, int type, const char *value)
{
	dbus_uint16_t uint16;

	switch (type) {
		case DBUS_TYPE_UINT16:
			uint16 = strtoul (value, NULL, 0);
			dbus_message_iter_append_basic (iter, DBUS_TYPE_UINT16, &uint16);
			break;
		case DBUS_TYPE_STRING:
			dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &value);
			break;
		default:
			fprintf (stderr, " Unsupported data type %c\n",  (char) type);
			exit (1);
	}
}

static int type_from_name (const char *arg)
{
	int type;
	if (!strcmp (arg, "string"))
		type = DBUS_TYPE_STRING;
	else if (!strcmp (arg, "uint16"))
		type = DBUS_TYPE_UINT16;
	else {
		fprintf (stderr, "Unknown type \"%s\"\n", arg);
		exit (1);
	}
	return type;
}

int main (int argc, char *argv[])
{
	DBusConnection *connection;
	DBusError error;
	DBusMessage *message;
	int print_reply = TRUE;
	int print_reply_literal;
	int reply_timeout;
	DBusMessageIter iter;
	int i;
	DBusBusType type = DBUS_BUS_SESSION;
	const char *name = NULL;
	const char *dest = "org.ganesha.nfsd";
	const char *path = "/org/ganesha/nfsd/ExportMgr";
	int session_or_system = TRUE;
	int message_type = DBUS_MESSAGE_TYPE_SIGNAL;
	print_reply_literal = FALSE;
	reply_timeout = -1;

	type = DBUS_BUS_SYSTEM;
	message_type = DBUS_MESSAGE_TYPE_METHOD_CALL;

	i = 2;
	name = argv[1];

	dbus_error_init (&error);
	connection = dbus_bus_get (type, &error);
	if (connection == NULL) {
		dbus_error_free (&error);
		exit (1);
	}


	char *last_dot;
	last_dot = strrchr (name, '.');

	if (last_dot == NULL) {
		fprintf (stderr, "Must use org.mydomain.Interface.Method notation, no dot in \"%s\"\n",
				name);
		exit (1);
	}

	*last_dot = '\0';
	message = dbus_message_new_method_call (NULL,
			path,
			name,
			last_dot + 1);
	dbus_message_set_auto_start (message, TRUE);
	if (message == NULL) {
		fprintf (stderr, "Couldn't allocate D-Bus message\n");
		exit (1);
	}

	if (dest && !dbus_message_set_destination (message, dest)) {
		fprintf (stderr, "Not enough memory\n");
		exit (1);
	}

	dbus_message_iter_init_append (message, &iter);
	while (i < argc) {
		char *arg;
		char *c;
		int type;
		int container_type;
		DBusMessageIter *target_iter;
		DBusMessageIter container_iter;

		type = DBUS_TYPE_INVALID;
		arg = argv[i++];
		c = strchr (arg, ':');
		if (c == NULL) {
			fprintf (stderr, "%s: Data item \"%s\" is badly formed\n", argv[0], arg);
			exit (1);
		}

		*(c++) = 0;
		container_type = DBUS_TYPE_INVALID;
		type = type_from_name (arg);
		target_iter = &iter;
		append_arg (target_iter, type, c);

		if (container_type != DBUS_TYPE_INVALID) {
			dbus_message_iter_close_container (&iter,
					&container_iter);
		}
	}

	if (print_reply) {
		DBusMessage *reply;
		dbus_error_init (&error);
		reply = dbus_connection_send_with_reply_and_block (connection,
				message, reply_timeout,
				&error);
		if (dbus_error_is_set (&error)) {
			fprintf (stderr, "Error %s: %s\n",
					error.name,
					error.message);
			exit (1);
		}

		if (reply) {
			//print_message (reply, print_reply_literal);
			dbus_message_unref (reply);
		}
	} else {
		dbus_connection_send (connection, message, NULL);
		dbus_connection_flush (connection);
	}

	dbus_message_unref (message);
	dbus_connection_unref (connection);
	exit (0);
}


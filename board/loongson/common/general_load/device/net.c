#include <stdlib.h>
#include "net.h"

static int m__net_gl_read(struct net_gl_desc* self, int proto,
		char* filepath, u64 size, u64* retsize, void* buf)
{
	int ret;

	env_set("netretry", "yes");
	env_set("ethrotate", "yes");

	copy_filename(net_boot_file_name, filepath,
		      sizeof(net_boot_file_name));
	net_server_ip = self->ip;

	ret = net_loop(proto);
	if (ret > 0)
	{
		*retsize = ret;
		ret = 0;
	}
	return ret;
}

struct net_gl_desc* net_gl_init(char* ip)
{
	struct net_gl_desc* desc = calloc(1, sizeof(*desc));
	if (desc == NULL)
		return NULL;

	desc->ip = string_to_ip(ip);
	if (desc->ip.s_addr == 0)
	{
		free(desc);
		return NULL;
	}

	desc->read = m__net_gl_read;
	return desc;
}

void net_gl_destroy(struct net_gl_desc* desc)
{
	if (desc != NULL)
		free(desc);
}


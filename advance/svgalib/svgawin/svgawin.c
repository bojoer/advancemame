#include "svgalib.h"

#include "svgawin.h"

/**
 * If defined use the GIVEIO mode to access the IO port.
 */
#define USE_GIVEIO

/**
 * If defined use the TOTALIO mode to access the IO port.
 */
/* #define USE_TOTALIO */

/**************************************************************************/
/* misc */

void adv_svgalib_enable(void) {
}

void adv_svgalib_disable(void) {
}

void adv_svgalib_usleep(unsigned n) {
	Sleep(n/1000);
}

/**************************************************************************/
/* ioctl */

static HANDLE the_handle = INVALID_HANDLE_VALUE;

int adv_svgalib_ioctl(unsigned code, void* input, unsigned input_size, void* output, unsigned output_size)
{
	DWORD returned;

	if (the_handle == INVALID_HANDLE_VALUE) {
		adv_svgalib_log("svgalib: handle not opened for ioctl %08x\n", code);
		return -1;
	}

	if (!DeviceIoControl(the_handle, code, input, input_size, output, output_size, &returned, 0)) {
		return -1;
	}

	if (returned != output_size) {
		adv_svgalib_log("svgalib: invalid output size for ioctl %08x\n", code);
		return -1;
	}

	return 0;
}

/**************************************************************************/
/* port */

#if defined(USE_GIVEIO) || defined(USE_TOTALIO)
unsigned char adv_svgalib_inportb(unsigned port)
{
	unsigned char rv;
	__asm__ __volatile__ ("inb %%dx, %0"
		: "=a" (rv)
		: "d" (port));
	return rv;
}

unsigned short adv_svgalib_inportw(unsigned port)
{
	unsigned short rv;
	__asm__ __volatile__ ("inw %%dx, %0"
		: "=a" (rv)
		: "d" (port));
	return rv;
}

unsigned adv_svgalib_inportl(unsigned port)
{
	unsigned rv;
	__asm__ __volatile__ ("inl %%dx, %0"
		: "=a" (rv)
		: "d" (port));
	return rv;
}

void adv_svgalib_outportb(unsigned port, unsigned char data)
{
	__asm__ __volatile__ ("outb %1, %%dx"
		:
		: "d" (port),
		"a" (data));
}

void adv_svgalib_outportw(unsigned port, unsigned short data)
{
	__asm__ __volatile__ ("outw %1, %%dx"
		:
		: "d" (port),
		"a" (data));
}

void adv_svgalib_outportl(unsigned port, unsigned data)
{
	__asm__ __volatile__ ("outl %1, %%dx"
		:
		: "d" (port),
		"a" (data));
}

#else

void adv_svgalib_outportb(unsigned port, unsigned char data)
{
	SVGALIB_PORT_WRITE_IN in;

	in.port = port;
	in.size = 1;
	in.data = data;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_WRITE failed, GetLastError() = %d\n", (unsigned)(unsigned)GetLastError());
	}
}

void adv_svgalib_outportw(unsigned port, unsigned short data)
{
	SVGALIB_PORT_WRITE_IN in;

	in.port = port;
	in.size = 2;
	in.data = data;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_WRITE failed, GetLastError() = %d\n", (unsigned)GetLastError());
	}
}

void adv_svgalib_outportl(unsigned port, unsigned data)
{
	SVGALIB_PORT_WRITE_IN in;

	in.port = port;
	in.size = 4;
	in.data = data;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_WRITE failed, GetLastError() = %d\n", (unsigned)GetLastError());
	}
}

unsigned char adv_svgalib_inportb(unsigned port)
{
	SVGALIB_PORT_READ_IN in;
	SVGALIB_PORT_READ_OUT out;

	in.port = port;
	in.size = 1;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_READ failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return 0;
	}

	return out.data;
}

unsigned short adv_svgalib_inportw(unsigned port)
{
	SVGALIB_PORT_READ_IN in;
	SVGALIB_PORT_READ_OUT out;

	in.port = port;
	in.size = 2;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_READ failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return 0;
	}

	return out.data;
}

unsigned adv_svgalib_inportl(unsigned port)
{
	SVGALIB_PORT_READ_IN in;
	SVGALIB_PORT_READ_OUT out;

	in.port = port;
	in.size = 4;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_READ failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return 0;
	}

	return out.data;
}
#endif

/**************************************************************************/
/* pci */

int adv_svgalib_pci_bus_max(unsigned* bus_max)
{
	SVGALIB_PCI_BUS_OUT out;
	
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_BUS, 0, 0, &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_BUS failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	*bus_max = out.bus;
	return 0;
}

int adv_svgalib_pci_read_byte(unsigned bus_device_func, unsigned reg, unsigned char* value) 
{
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 1;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_READ failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_read_word(unsigned bus_device_func, unsigned reg, unsigned short* value) {
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 2;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_READ failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_read_dword(unsigned bus_device_func, unsigned reg, unsigned* value) {
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 4;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_READ failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_read_dword_nolog(unsigned bus_device_func, unsigned reg, unsigned* value) {
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 4;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_write_byte(unsigned bus_device_func, unsigned reg, unsigned char value) {
	SVGALIB_PCI_WRITE_IN in;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 1;
	in.data = value;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_WRITE failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	return 0;
}

int adv_svgalib_pci_write_word(unsigned bus_device_func, unsigned reg, unsigned short value) {
	SVGALIB_PCI_WRITE_IN in;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 2;
	in.data = value;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_WRITE failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	return 0;
}

int adv_svgalib_pci_write_dword(unsigned bus_device_func, unsigned reg, unsigned value) {
	SVGALIB_PCI_WRITE_IN in;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 4;
	in.data = value;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_WRITE failed, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	return 0;
}

/**************************************************************************/
/* device */

/**
 * The video board bus number.
 */
static unsigned the_bus;

static int bus_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg) {
	unsigned dw;
	unsigned base_class;

	if (adv_svgalib_pci_read_dword(bus_device_func,0x8,&dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	*(unsigned*)_arg = bus_device_func;

	return 1;
}

int adv_svgalib_open(void) {
	int r;
	unsigned bus_device_func;
	SVGALIB_VERSION_OUT version;

	the_handle = CreateFile("\\\\.\\SVGALIB", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (the_handle == INVALID_HANDLE_VALUE) {
		adv_svgalib_log("svgalib: error opening the SVGAWIN device, GetLastError() = %d\n", (unsigned)GetLastError());
		return -1;
	}

	/* check the version */
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_VERSION,0,0,&version,sizeof(version)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_VERSION failed, GetLastError() = %d\n", (unsigned)GetLastError());
		CloseHandle(the_handle);
		the_handle = INVALID_HANDLE_VALUE;
		return -1;
	}
	if (version.version < SVGALIB_VERSION) {
		adv_svgalib_log("svgalib: invalid SVGALIB version %08x. Minimun required is %08x.\n", (unsigned)version.version, (unsigned)SVGALIB_VERSION);
		CloseHandle(the_handle);
		the_handle = INVALID_HANDLE_VALUE;
		return -1;
	}

#ifdef USE_TOTALIO
	/* get the permission on all the IO port */
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_TOTALIO_ON,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_TOTALIO_ON failed, GetLastError() = %d\n", (unsigned)GetLastError());
		CloseHandle(the_handle);
		the_handle = INVALID_HANDLE_VALUE;
		return -1;
	}
#endif

#ifdef USE_GIVEIO
	/* get the permission on all the IO port */
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_GIVEIO_ON,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_GIVEIO_ON failed, GetLastError() = %d\n", (unsigned)GetLastError());
		CloseHandle(the_handle);
		the_handle = INVALID_HANDLE_VALUE;
		return -1;
	}
#endif

	/* search the bus on which is the first video board */
	r = adv_svgalib_pci_scan_device(bus_callback,&bus_device_func);
	if (r != 0) {
		/* found */
		the_bus = (bus_device_func >> 8) & 0xFF;
	} else {
		adv_svgalib_log("svgalib: adv_svgalib_pci_scan_device has not found a video board\n");
		the_bus = 0;
	}

	return 0;
}

void adv_svgalib_close(void) {
#ifdef USE_GIVEIO
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_GIVEIO_OFF,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_GIVEIO_OFF failed, GetLastError() = %d\n", (unsigned)GetLastError());
	}
#endif

#ifdef USE_TOTALIO
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_TOTALIO_OFF,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_TOTALIO_OFF failed, GetLastError() = %d\n", (unsigned)GetLastError());
	}
#endif

	CloseHandle(the_handle);
	the_handle = INVALID_HANDLE_VALUE;
}

/**************************************************************************/
/* mmap */

void* adv_svgalib_mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset) {
	SVGALIB_MAP_IN in;
	SVGALIB_MAP_OUT out;

	(void)prot;
	(void)fd;

	if ((flags & MAP_FIXED) != 0)
		return MAP_FAILED;

	in.address.QuadPart = offset;
	in.bus = the_bus;
	in.size = length;

	adv_svgalib_log("svgalib: mapping address %08x, size %d, bus %d\n", offset, length, the_bus);

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_MAP, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_MAP failed, GetLastError() = %d\n", (unsigned)GetLastError());	
		return MAP_FAILED;
	}

	adv_svgalib_log("svgalib: mapped pointer %08x\n", (unsigned)out.address);

	return out.address;
}

int adv_svgalib_munmap(void* start, unsigned length) {
	SVGALIB_UNMAP_IN in;

	adv_svgalib_log("svgalib: unmapping pointer %08x, size %d\n", (unsigned)start, length);

	in.address = start;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_UNMAP, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_UNMAP failed, GetLastError() = %d\n", (unsigned)(unsigned)GetLastError());	
		return -1;
	}

	return 0;
}

/**************************************************************************/
/* iopl */

int adv_svgalib_iopl(int perm) {
	(void)perm;
	return 0;
}

/***************************************************************************/
/* vga */

void __svgalib_delay(void) {
	__asm__ __volatile__ (
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		:
		:
		: "cc", "%eax"
	);
}

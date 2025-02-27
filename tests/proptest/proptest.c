/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Paulo Zanoni <paulo.r.zanoni@intel.com>
 *
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xf86drm.h"
#include "xf86drmMode.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
static inline int64_t U642I64(uint64_t val)
{
	return (int64_t)*((int64_t *)&val);
}

int fd;
drmModeResPtr res = NULL;

const char *connector_type_str(uint32_t type)
{
	switch (type) {
	case DRM_MODE_CONNECTOR_Unknown:
		return "Unknown";
	case DRM_MODE_CONNECTOR_VGA:
		return "VGA";
	case DRM_MODE_CONNECTOR_DVII:
		return "DVI-I";
	case DRM_MODE_CONNECTOR_DVID:
		return "DVI-D";
	case DRM_MODE_CONNECTOR_DVIA:
		return "DVI-A";
	case DRM_MODE_CONNECTOR_Composite:
		return "Composite";
	case DRM_MODE_CONNECTOR_SVIDEO:
		return "SVIDEO";
	case DRM_MODE_CONNECTOR_LVDS:
		return "LVDS";
	case DRM_MODE_CONNECTOR_Component:
		return "Component";
	case DRM_MODE_CONNECTOR_9PinDIN:
		return "9PinDin";
	case DRM_MODE_CONNECTOR_DisplayPort:
		return "DisplayPort";
	case DRM_MODE_CONNECTOR_HDMIA:
		return "HDMI-A";
	case DRM_MODE_CONNECTOR_HDMIB:
		return "HDMI-B";
	case DRM_MODE_CONNECTOR_TV:
		return "TV";
	case DRM_MODE_CONNECTOR_eDP:
		return "eDP";
	default:
		return "Invalid";
	}
}

/* dump_blob and dump_prop shamelessly copied from ../modetest/modetest.c */
static void
dump_blob(uint32_t blob_id)
{
	uint32_t i;
	unsigned char *blob_data;
	drmModePropertyBlobPtr blob;

	blob = drmModeGetPropertyBlob(fd, blob_id);
	if (!blob) {
		printf("\n");
		return;
	}

	blob_data = blob->data;

	for (i = 0; i < blob->length; i++) {
		if (i % 16 == 0)
			printf("\n\t\t\t");
		printf("%.2hhx", blob_data[i]);
	}
	printf("\n");

	drmModeFreePropertyBlob(blob);
}

static void
dump_prop(uint32_t prop_id, uint64_t value)
{
	int i;
	drmModePropertyPtr prop;

	prop = drmModeGetProperty(fd, prop_id);

	printf("\t%d", prop_id);
	if (!prop) {
		printf("\n");
		return;
	}

	printf(" %s:\n", prop->name);

	printf("\t\tflags:");
	if (prop->flags & DRM_MODE_PROP_PENDING)
		printf(" pending");
	if (prop->flags & DRM_MODE_PROP_SIGNED)
		printf(" signed");
	if (prop->flags & DRM_MODE_PROP_IMMUTABLE)
		printf(" immutable");
	if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE))
		printf(" signed range");
	if (drm_property_type_is(prop, DRM_MODE_PROP_RANGE))
		printf(" range");
	if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM))
		printf(" enum");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BITMASK))
		printf(" bitmask");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB))
		printf(" blob");
	if (drm_property_type_is(prop, DRM_MODE_PROP_OBJECT))
		printf(" object");
	printf("\n");


	if (drm_property_type_is(prop, DRM_MODE_PROP_SIGNED_RANGE)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_values; i++)
			printf(" %"PRId64, U642I64(prop->values[i]));
		printf("\n");
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_RANGE)) {
		printf("\t\tvalues:");
		if (prop->flags & DRM_MODE_PROP_SIGNED) {
			for (i = 0; i < prop->count_values; i++)
				printf(" %"PRId64, U642I64(prop->values[i]));
		} else {
			for (i = 0; i < prop->count_values; i++)
				printf(" %"PRIu64, prop->values[i]);
		}
		printf("\n");
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM)) {
		printf("\t\tenums:");
		for (i = 0; i < prop->count_enums; i++)
			printf(" %s=%llu", prop->enums[i].name,
			       prop->enums[i].value);
		printf("\n");
	} else if (drm_property_type_is(prop, DRM_MODE_PROP_BITMASK)) {
		printf("\t\tvalues:");
		for (i = 0; i < prop->count_enums; i++)
			printf(" %s=0x%llx", prop->enums[i].name,
			       (1LL << prop->enums[i].value));
		printf("\n");
	} else {
		assert(prop->count_enums == 0);
	}

	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB)) {
		printf("\t\tblobs:\n");
		for (i = 0; i < prop->count_blobs; i++)
			dump_blob(prop->blob_ids[i]);
		printf("\n");
	} else {
		assert(prop->count_blobs == 0);
	}

	printf("\t\tvalue:");
	if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB))
		dump_blob(value);
	else
		printf(" %"PRIu64"\n", value);

	drmModeFreeProperty(prop);
}

static void listObjectProperties(uint32_t id, uint32_t type)
{
	unsigned int i;
	drmModeObjectPropertiesPtr props;

	props = drmModeObjectGetProperties(fd, id, type);

	if (!props) {
		printf("\tNo properties: %s.\n", strerror(errno));
		return;
	}

	for (i = 0; i < props->count_props; i++)
		dump_prop(props->props[i], props->prop_values[i]);

	drmModeFreeObjectProperties(props);
}

static void listConnectorProperties(void)
{
	int i;
	drmModeConnectorPtr c;

	for (i = 0; i < res->count_connectors; i++) {
		c = drmModeGetConnector(fd, res->connectors[i]);

		if (!c) {
			fprintf(stderr, "Could not get connector %u: %s\n",
				res->connectors[i], strerror(errno));
			continue;
		}

		printf("Connector %u (%s-%u)\n", c->connector_id,
		       connector_type_str(c->connector_type),
		       c->connector_type_id);

		listObjectProperties(c->connector_id,
				     DRM_MODE_OBJECT_CONNECTOR);

		drmModeFreeConnector(c);
	}
}

static void listCrtcProperties(void)
{
	int i;
	drmModeCrtcPtr c;

	for (i = 0; i < res->count_crtcs; i++) {
		c = drmModeGetCrtc(fd, res->crtcs[i]);

		if (!c) {
			fprintf(stderr, "Could not get crtc %u: %s\n",
				res->crtcs[i], strerror(errno));
			continue;
		}

		printf("CRTC %u\n", c->crtc_id);

		listObjectProperties(c->crtc_id, DRM_MODE_OBJECT_CRTC);

		drmModeFreeCrtc(c);
	}
}

static void listAllProperties(void)
{
	listConnectorProperties();
	listCrtcProperties();
}

static int setProperty(char *argv[])
{
	uint32_t obj_id, obj_type, prop_id;
	uint64_t value;

	obj_id = atoi(argv[1]);

	if (!strcmp(argv[2], "connector")) {
		obj_type = DRM_MODE_OBJECT_CONNECTOR;
	} else if (!strcmp(argv[2], "crtc")) {
		obj_type = DRM_MODE_OBJECT_CRTC;
	} else {
		fprintf(stderr, "Invalid object type.\n");
		return 1;
	}

	prop_id = atoi(argv[3]);
	value = atoll(argv[4]);

	return drmModeObjectSetProperty(fd, obj_id, obj_type, prop_id, value);
}

static void printUsage(void)
{
	printf("Usage:\n"
"  proptest\n"
"  proptest [obj id] [obj type] [prop id] [value]\n"
"\n"
"The first form just prints all the existing properties. The second one is\n"
"used to set the value of a specified property. The object type can be one of\n"
"the following strings:\n"
"  connector crtc\n"
"\n"
"Example:\n"
"  proptest 7 connector 2 1\n"
"will set property 2 of connector 7 to 1\n");
}

int main(int argc, char *argv[])
{
	char *modules[] = { "i915", "radeon", "nouveau", "vmwgfx", "omapdrm", "msm", "rockchip"};
	unsigned int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(modules); i++){
		fd = drmOpen(modules[i], NULL);
		if (fd >= 0) {
			printf("Module %s loaded.\n", modules[i]);
			break;
		}
	}

	if (i == ARRAY_SIZE(modules)) {
		fprintf(stderr, "Failed to load drm modules.\n");
		return 1;
	}

	res = drmModeGetResources(fd);
	if (!res) {
		fprintf(stderr, "Failed to get resources: %s\n",
			strerror(errno));
		ret = 1;
		goto done;
	}

	if (argc < 2) {
		listAllProperties();
	} else if (argc == 5) {
		ret = setProperty(argv);
	} else {
		printUsage();
		ret = 1;
	}

	drmModeFreeResources(res);
done:
	drmClose(fd);
	return ret;
}

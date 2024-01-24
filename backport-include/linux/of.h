#ifndef _BACKPORT_LINUX_OF_H
#define _BACKPORT_LINUX_OF_H
#include_next <linux/of.h>

#ifndef BPM_OF_MODALIAS_NODE_RENAMED
#define of_alias_from_compatible of_modalias_node
#endif

#ifndef BPM_OF_PROPERTY_PRESENT_AVAILABLE
/**
 * of_property_present - Test if a property is present in a node
 * @np:		device node to search for the property.
 * @propname:	name of the property to be searched.
 *
 * Test for a property present in a device node.
 *
 * Return: true if the property exists false otherwise.
 */
static inline bool of_property_present(const struct device_node *np, const char *propname)
{
	return of_property_read_bool(np, propname);
}
#endif

#endif

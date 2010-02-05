/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFCNFG_H_
#define CFCNFG_H_
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfctrl.h>

struct cfcnfg;

/* Types of physical layers defined in CAIF Stack */
enum cfcnfg_phy_type {
	/* Fragmented frames physical interface */
	CFPHYTYPE_FRAG = 1,
	/* Generic CAIF physical interface */
	CFPHYTYPE_CAIF,
	CFPHYTYPE_MAX
};

/* Physical preference - HW Abstraction */
enum cfcnfg_phy_preference {
	/* Default physical interface */
	CFPHYPREF_UNSPECIFIED,
	/* Default physical interface for low-latency traffic */
	CFPHYPREF_LOW_LAT,
	/* Default physical interface for high-bandwidth traffic */
	CFPHYPREF_HIGH_BW,
	/* TEST only Loopback interface simulating modem responses */
	CFPHYPREF_LOOP
};

/*
 * Create the CAIF configuration object.
 * @return The created instance of a CFCNFG object.
 */
struct cfcnfg *cfcnfg_create(void);

/* Remove the CFCNFG object */
void cfcnfg_remove(struct cfcnfg *cfg);

/*
 * Adds a physical layer to the CAIF stack.
 * @param cnfg Pointer to a CAIF configuration object, created by
 *				cfcnfg_create().
 * @param phy_type Specifies the type of physical interface, e.g.
 *		    CFPHYTYPE_FRAG.
 * @param phy_layer Specify the physical layer. The transmit function
 *		    MUST be set in the structure.
 * @param phyid [out] The assigned physical ID for this layer,
 *		      used in \ref cfcnfg_add_adapt_layer to specify
 *		      PHY for the link.
 */

void
cfcnfg_add_phy_layer(struct cfcnfg *cnfg, enum cfcnfg_phy_type phy_type,
		     void *dev, struct layer *phy_layer, uint16 *phyid,
		     enum cfcnfg_phy_preference pref,
		     bool fcs, bool stx);

/*
 * Deletes an phy layer from the CAIF stack.
 *
 * @param cnfg Pointer to a CAIF configuration object, created by
 *			   cfcnfg_create().
 * @param phy_layer Adaptation layer to be removed.
 * @return 0 on success.
 */
int cfcnfg_del_phy_layer(struct cfcnfg *cnfg, struct layer *phy_layer);


/*
 * Deletes an adaptation layer from the CAIF stack.
 *
 * @param cnfg Pointer to a CAIF configuration object, created by
 *			   cfcnfg_create().
 * @param adap_layer Adaptation layer to be removed.
 * @return 0 on success.
 */
int cfcnfg_del_adapt_layer(struct cfcnfg *cnfg, struct layer *adap_layer);


/*
 * Adds an adaptation layer to the CAIF stack.
 * The adaptation Layer is where the interface to application or higher-level
 * driver functionality is implemented.
 *
 * @param cnfg		Pointer to a CAIF configuration object, created by
 *				cfcnfg_create().
 * @param param		Link setup parameters.
 * @param adap_layer	Specify the adaptation layer; the receive and flow-control
			functions MUST be set in the structure.
 * @return		true on success, false upon failure.
 */
bool
cfcnfg_add_adaptation_layer(struct cfcnfg *cnfg,
			    struct cfctrl_link_param *param,
			    struct layer *adap_layer);
/*
 * Get physical ID, given type.
 * @return Returns one of the physical interfaces matching the given type.
 *	   Zero if no match is found.
 */
struct dev_info *cfcnfg_get_phyid(struct cfcnfg *cnfg,
		     enum cfcnfg_phy_preference phy_pref);
int cfcnfg_get_named(struct cfcnfg *cnfg, char *name);

#endif				/* CFCNFG_H_ */

#define PJ_LIB__
#include "proj_internal.h"
#include "projects.h"

PROJ_HEAD(vgridshift, "Vertical grid shift");


static XYZ forward_3d(LPZ lpz, PJ *P) {
    PJ_TRIPLET point;
    point.lpz = lpz;

    if (P->vgridlist_geoid != NULL) {
        /* Only try the gridshift if at least one grid is loaded,
         * otherwise just pass the coordinate through unchanged. */
        point.xyz.z -= proj_vgrid_value(P, point.lp);
    }

    return point.xyz;
}


static LPZ reverse_3d(XYZ xyz, PJ *P) {
    PJ_TRIPLET point;
    point.xyz = xyz;

    if (P->vgridlist_geoid != NULL) {
        /* Only try the gridshift if at least one grid is loaded,
         * otherwise just pass the coordinate through unchanged. */
        point.xyz.z += proj_vgrid_value(P, point.lp);
    }

    return point.lpz;
}


static PJ_OBS forward_obs(PJ_OBS obs, PJ *P) {
    PJ_OBS point;
    point.coo.xyz = forward_3d (obs.coo.lpz, P);
    return point;
}

static PJ_OBS reverse_obs(PJ_OBS obs, PJ *P) {
    PJ_OBS point;
    point.coo.lpz = reverse_3d (obs.coo.xyz, P);
    return point;
}


PJ *PROJECTION(vgridshift) {

   if (!pj_param(P->ctx, P->params, "tgrids").i) {
        proj_log_error(P, "vgridshift: +grids parameter missing.");
        return pj_default_destructor(P, PJD_ERR_NO_ARGS);
    }

    /* Build gridlist. P->vgridlist_geoid can be empty if +grids only ask for optional grids. */
    proj_vgrid_init(P, "grids");

    /* Was gridlist compiled properly? */
    if ( proj_errno(P) ) {
        proj_log_error(P, "vgridshift: could not find required grid(s).");
        return pj_default_destructor(P, PJD_ERR_FAILED_TO_LOAD_GRID);
    }

    P->fwdobs = forward_obs;
    P->invobs = reverse_obs;
    P->fwd3d  = forward_3d;
    P->inv3d  = reverse_3d;
    P->fwd    = 0;
    P->inv    = 0;

    P->left  = PJ_IO_UNITS_RADIANS;
    P->right = PJ_IO_UNITS_RADIANS;

    return P;
}


#ifndef PJ_SELFTEST
/* selftest stub */
int pj_vgridshift_selftest (void) {return 0;}
#else
int pj_vgridshift_selftest (void) {
    PJ *P;
    PJ_OBS expect, a, b;
    double dist;
    int failures = 0;

    /* fail on purpose: +grids parameter is mandatory*/
    P = proj_create(PJ_DEFAULT_CTX, "+proj=vgridshift");
    if (0!=P) {
        proj_destroy (P);
        return 99;
    }

    /* fail on purpose: open non-existing grid */
    P = proj_create(PJ_DEFAULT_CTX, "+proj=vgridshift +grids=nonexistinggrid.gtx");
    if (0!=P) {
        proj_destroy (P);
        return 999;
    }

    /* Failure most likely means the grid is missing */
    P = proj_create(PJ_DEFAULT_CTX, "+proj=vgridshift +grids=egm96_15.gtx +ellps=GRS80");
    if (0==P)
        return 10;

    a = proj_obs_null;
    a.coo.lpz.lam = PJ_TORAD(12.5);
    a.coo.lpz.phi = PJ_TORAD(55.5);

    dist = proj_roundtrip (P, PJ_FWD, 1, a.coo);
    if (dist > 0.00000001)
        return 1;

    expect = a;
    /* Appears there is a difference between the egm96_15.gtx distributed by OSGeo4W,  */
    /* and the one from http://download.osgeo.org/proj/vdatum/egm96_15/egm96_15.gtx    */
    /* Was: expect.coo.lpz.z   = -36.021305084228515625;  (download.osgeo.org)         */
    /* Was: expect.coo.lpz.z   = -35.880001068115234000;  (OSGeo4W)                    */
    /* This is annoying, but must be handled elsewhere. So for now, we check for both. */
    expect.coo.lpz.z   = -36.021305084228516;
    failures = 0;
    b = proj_trans_obs(P, PJ_FWD, a);
    if (proj_xyz_dist(expect.coo.xyz, b.coo.xyz) > 1e-4)  failures++;
    expect.coo.lpz.z   = -35.880001068115234000;
    if (proj_xyz_dist(expect.coo.xyz, b.coo.xyz) > 1e-4)  failures++;
    if (failures > 1)
        return 2;
    
    proj_destroy (P);

    return 0;
}
#endif
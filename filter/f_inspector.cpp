#include "stdafx.h"

// Copyright(c) 2012 Yohei Matsumoto, Tokyo University of Marine
// Science and Technology, All right reserved. 

// f_inspector.cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Publica License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_inspector.cpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_inspector.cpp.  If not, see <http://www.gnu.org/licenses/>. 
#include <Windowsx.h>
#include <cstdio>

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
using namespace std;

#include <opencv2/opencv.hpp>
using namespace cv;

#include "../util/aws_sock.h"
#include "../util/aws_thread.h"

#include "../util/aws_cminpack.h"
#include "../util/coord.h"
#include "../util/c_ship.h"
#include "../util/c_clock.h"

//#include "../util/c_nmeadec.h"
#include "../channel/ch_base.h"
#include "../channel/ch_image.h"

#include "f_inspector.h"

const DWORD ModelVertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;  

//About the parameter estimation
//m: 2d point projected
//M: 3d point in object coordinate
//r: rotation params (including world/camera rotation)
//   vector in R^3 
//t: translation params (including world/camera translation)
//   vector in R^3
//p: projection params
//   vector in R^4 (focal length and principal point for both x,y direction)
//k: distortion params
//   vector in R^6
//
//The projection function is
//
//m = D(P(T(R(M;r);t),p),k)
//
//Here I ommit the indices of the points and objects
//
//The estimation of parameters of camera and objects means the optimization,
//
//    min Sum(m-m')^2
// [r, t, p, k]^T for all points
//
//First, to enable iterative minimization, linearize the function D around [r,t,p,k]^T
//
//D(P(T(R(M;r+dr);t+dt);p+dp)k+dk) 
//  <=> J[dr, dt, dp, dk]^T + D(P(T(R(M;r);t);p)k)
//
//J is the Jacobian. Then we minimize
//
//     min Sum{m - J[dr, dt, dp, dk]^T - D(P(T(R(M;r);t);p)k)}^2
// [dr, dt, dp, dk]^T for all points
//
//and update parameters with
//
//[r, t, p, k]^T <= [r, t, p, k]^T + [dr, dt, dp, dk]^T
//
//The [dr ,dt, dp, dk]^T is actually,
//
//Sum{2J^T{m - J[dr, dt, dp, dk]^T - D(P(T(R(M;r);t);p)k)}=0 (the derivative equals zero)
//Sum {J^TJ [dr, dt, dp, dk]^T} = Sum {J^Tm - D(P(T(R(M;r);t);p)k)}
//[dr, dt, dp, dk]^T = Sum {J^TJ}^(-1) Sum J^T{m - D(P(T(R(M;r);t);p)k)} 
//
//yes,it's Gauss-Newton method. And the Jacobian can be calculated as,
//
//J=dD/d[r, t, p, k]^T = [dD/dr, dD/dt, dD/dp, dD/dk]
//
//dD/dr=dD/dP * dP/dT * dT/dR * dR/dr
//dD/dt=dD/dP * dP/dT * dT/dt
//dD/dp=dD/dP * dP/dp
//dD/dk=dD/dk
//
//The Jacobian can also be calculated with cv::projectPoints

//////////////////////////////////////////////////////////////// helper function
void get_cursor_point(vector<Point2f> & pt2ds, float x, float y, int & idx, double & dist)
{
	dist = DBL_MAX;
	idx = -1;
	for(int i = 0; i < pt2ds.size(); i++)
	{
		Point2f & pt = pt2ds[i];
		double dx = pt.x -  x;
		double dy = pt.y -  y;
		double d = dx * dx + dy * dy;
		if(d < dist){
			dist = d;
			idx = i;
		}
	}
	dist = sqrt(dist);
}

void render_prjpts(s_model & mdl, vector<Point2f> & pt2dprj,
	LPDIRECT3DDEVICE9 pd3dev, c_d3d_dynamic_text * ptxt, LPD3DXLINE pline,
	int pttype, int state, int cur_point)
{
	// state 0: NORMAL -> 127
	// state 1: STRONG -> 255
	// state 2: GRAY -> 127,127,127
	// state 3: WHITE -> 255,255,255
	vector<s_edge> & edges = mdl.edges;

	pttype %= 12; // {sq, dia, x, cross} x {red, green, blue}
	int shape = pttype % 4;
	int val;
	if(state == 0 || state == 2){
		val = 128;
	}else if(state == 1 || state == 3){
		val = 255;
	}

	D3DCOLOR color;
	if(state < 2){
		switch(pttype / 4){
		case 0:
			color = D3DCOLOR_RGBA(val, 0, 0, 255);
			break;
		case 1:
			color = D3DCOLOR_RGBA(0, val, 0, 255);
			break;
		case 2:
			color = D3DCOLOR_RGBA(0, 0, val, 255);
			break;
		}
	}else{
		color = D3DCOLOR_RGBA(val, val, val, 255);
	}
	pline->SetAntialias(TRUE);

	pline->Begin();
	for(int iedge = 0; iedge < edges.size(); iedge++){
		D3DXVECTOR2 v[2];
		Point2f & pt1 = pt2dprj[edges[iedge].s];
		Point2f & pt2 = pt2dprj[edges[iedge].e];
		v[0] = D3DXVECTOR2(pt1.x, pt1.y);
		v[1] = D3DXVECTOR2(pt2.x, pt2.y);
		pline->Draw(v, 2, color);
	}
	D3DXVECTOR2 v[5];
	int size = 5;
	for(int ipt = 0; ipt < pt2dprj.size(); ipt++){
		Point2f & pt = pt2dprj[ipt];
		if(ipt == cur_point){
			v[0] = D3DXVECTOR2((float)(pt.x - 1.0), (float)(pt.y));
			v[1] = D3DXVECTOR2((float)(pt.x - 3.0), (float)(pt.y));
			pline->Draw(v, 2, color);
			v[0].x += 4.0;
			v[1].x += 4.0;
			pline->Draw(v, 2, color);
			v[0] = D3DXVECTOR2((float)(pt.x), (float)(pt.y - 1.0));
			v[1] = D3DXVECTOR2((float)(pt.x), (float)(pt.y - 3.0));
			pline->Draw(v, 2, color);
			v[0].y += 4.0;
			v[1].y += 4.0;
			pline->Draw(v, 2, color);
			continue;
		}

		switch(shape){
		case 0: // square
			xsquare(pline, pt, 2, color);
			break;
		case 1: // diamond
			xdiamond(pline, pt, 2, color);
			break;
		case 2: // X
			xdiagonal(pline, pt, 2, color);
			break;
		case 3:
			xcross(pline, pt, 2, color);
			break;
		}

		if(ptxt != NULL){
			char buf[10];
			sprintf(buf, "%d", ipt);
			ptxt->render(pd3dev, buf, pt.x, (float)(pt.y + 3.0), 1.0, 0.0, EDTC_CB, color); 
		}
	}
	pline->End();
}
///////////////////////////////////////////////////////////////// for cminpack
struct s_package {
	int num_models;
	vector<int> & num_pts;
	vector<vector<Point2f > > & p2d;
	vector<vector<Point2f > > p2dprj;
	vector<vector<Point3f > > & p3d;
	vector<Mat> & cam_int_tbl;
	vector<Mat> & cam_dist_tbl;
	Mat cam_int, cam_dist;
	s_package(int anum_models, vector<int> & anum_pts, 
		vector<vector<Point2f > > & ap2d,
		vector<vector<Point3f > > & ap3d, 
		vector<Mat> & acam_int_tbl, 
		vector<Mat> & acam_dist_tbl,
		Mat & acam_int, Mat & acam_dist): 
	num_models(anum_models), num_pts(anum_pts), p2d(ap2d), p3d(ap3d),
	cam_int_tbl(acam_int_tbl), cam_dist_tbl(acam_dist_tbl), 
	cam_int(acam_int), cam_dist(acam_dist){
	}
};

int prj_pause_and_cam(void * p, int m, int n, const __cminpack_real__ *x,
			__cminpack_real__ *fvec, int iflag)
{
	// input layout
	// x[0] : fx, x[1] : fy, x[2] : cx, x[3] : cy
	// x[4] : k1, x[5] : k2, x[6] : px, x[7] : py
	// x[8] : k3, x[9] : k4, x[10]: k5, x[11]: k6
	// x[12 ~] : rvec and tvec

	s_package * pkg = (s_package *) p;
	Mat & cam_int = pkg->cam_int;
	Mat & cam_dist = pkg->cam_dist;

	// loading projection matrix
	double * ptr = cam_int.ptr<double>(0);
	ptr[0] = x[0]; //fx
	ptr[2] = x[2]; // cx
	ptr[4] = x[1]; // fy
	ptr[5] = x[3]; // cy

	// loading distortino parameter
	ptr = cam_dist.ptr<double>(0);
	ptr[0] = x[4]; // k1
	ptr[1] = x[5]; // k2
	ptr[2] = x[6]; // px
	ptr[3] = x[7]; // py
	ptr[4] = x[8]; // k3
	ptr[5] = x[9]; // k4
	ptr[6] = x[10]; // k5
	ptr[7] = x[11]; // k6;

	const double * px = &(x[12]);
	double * pf = fvec;
	for(int im = 0; im < pkg->num_models; im++){
		Mat rvec = Mat(3, 1, CV_64FC1, (void*) px);
		Mat tvec = Mat(3, 1, CV_64FC1, (void*) (px+3));
		projectPoints(pkg->p3d[im], rvec, tvec, cam_int, 
			cam_dist, pkg->p2dprj[im]);
		for(int ipt = 0; ipt < pkg->num_pts[im]; ipt++){
			pf[0] = pkg->p2dprj[im][ipt].x - pkg->p2d[im][ipt].x;
			pf[1] = pkg->p2dprj[im][ipt].y - pkg->p2d[im][ipt].y;
			pf += 2;
		}
		px += 6;
	}
	return 0;
}

int prj_pause_and_cam_with_tbl(void * p, int m, int n, const __cminpack_real__ *x,
			__cminpack_real__ *fvec, int iflag)
{
	// input layout
	// x[0] : camera parameter table index
	// x[1 ~ ]: rvec and tvec

	s_package * pkg = (s_package *) p;
	Mat & cam_int = pkg->cam_int;
	Mat & cam_dist = pkg->cam_dist;

	// loading projection matrix
	double * ptr = cam_int.ptr<double>(0);
	int x0_l = max(0, (int) x[0]);  
	double rat = x[0] - (double) x0_l;
	double rat_i = 1.0 - rat;
	int x0_u = min((int)(pkg->cam_int_tbl.size() - 1), x0_l + 1);

	double * ptr_u = pkg->cam_int_tbl[x0_u].ptr<double>(0);
	double * ptr_l = pkg->cam_int_tbl[x0_l].ptr<double>(0);

	ptr[0] = rat_i * ptr_l[0] + rat * ptr_u[0]; //fx
	ptr[2] = rat_i * ptr_l[2] + rat * ptr_u[2]; // cx
	ptr[4] = rat_i * ptr_l[4] + rat * ptr_u[4]; // fy
	ptr[5] = rat_i * ptr_l[5] + rat * ptr_u[5]; // cy

	// loading distortion parameter
	ptr = cam_dist.ptr<double>(0);
	ptr_u = pkg->cam_dist_tbl[x0_u].ptr<double>(0);
	ptr_l = pkg->cam_dist_tbl[x0_l].ptr<double>(0);
	for(int i = 0; i < 8; i++){
		ptr[i] = rat_i * ptr_l[i] + rat * ptr_u[i];; // k1
	}
	
	const double * px = &(x[1]);
	double * pf = fvec;
	for(int im = 0; im < pkg->num_models; im++){
		Mat rvec = Mat(3, 1, CV_64FC1, (void*) px);
		Mat tvec = Mat(3, 1, CV_64FC1, (void*) (px+3));
		projectPoints(pkg->p3d[im], rvec, tvec, cam_int, 
			cam_dist, pkg->p2dprj[im]);
		for(int ipt = 0; ipt < pkg->num_pts[im]; ipt++){
			pf[0] = pkg->p2dprj[im][ipt].x - pkg->p2d[im][ipt].x;
			pf[1] = pkg->p2dprj[im][ipt].y - pkg->p2d[im][ipt].y;
			pf += 2;
		}
		px += 6;
	}
	return 0;
}

int prj_pause(void * p, int m, int n, const __cminpack_real__ *x,
			__cminpack_real__ *fvec, int iflag)
{
	s_package * pkg = (s_package *) p;
	Mat & cam_int = pkg->cam_int;
	Mat & cam_dist = pkg->cam_dist;

	const double * px = &(x[0]);
	double * pf = fvec;
	for(int im = 0; im < pkg->num_models; im++){
		Mat rvec = Mat(3, 1, CV_64FC1, (void*) px);
		Mat tvec = Mat(3, 1, CV_64FC1, (void*) (px+3));
		projectPoints(pkg->p3d[im], rvec, tvec, cam_int, 
			cam_dist, pkg->p2dprj[im]);
		for(int ipt = 0; ipt < pkg->num_pts[im]; ipt++){
			pf[0] = pkg->p2dprj[im][ipt].x - pkg->p2d[im][ipt].x;
			pf[1] = pkg->p2dprj[im][ipt].y - pkg->p2d[im][ipt].y;
			pf += 2;
		}
		px += 6;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////// struct s_model

double s_model::get_max_dist()
{
	float dx = (float)(xmax - xmin);
	float dy = (float)(ymax - ymin);
	float dz = (float)(zmax - zmin);

	return sqrt(dz * dz + dy * dy + dx * dx);
}

void s_model::proj(vector<Point2f> & pt2ds,  Mat & cam_int, Mat & cam_dist, Mat & rvec_cam, Mat & tvec_cam, 
		Mat & rvec_obj, Mat & tvec_obj)
{
	Mat rvec, tvec;
	composeRT(rvec_cam, tvec_cam, rvec_obj, tvec_obj, rvec, tvec);
	projectPoints(pts, rvec, tvec, cam_int, cam_dist, pt2ds);
}

bool s_model::load(const char * afname)
{
	fname = afname;
	FileStorage fs;
	fs.open(fname, FileStorage::READ);
	if(!fs.isOpened()){
		return false;
	}

	FileNode fn;

	fn = fs["ModelName"];
	string nameModel;
	if(fn.empty()){
		cerr << "Cannot find node ModelName." << endl;
		return false;
	}
	fn >> name;

	int numPoints;
	fn = fs["NumPoints"];
	if(fn.empty()){
		cerr << "Cannot find node NumPoints." << endl;
		return false;
	}
	fn >> numPoints;

	int numEdges; 
	fn = fs["NumEdges"];
	if(fn.empty()){
		cerr << "Cannot find node NumEdges." << endl;
		return false;
	}
	fn >> numEdges;

	fn = fs["Points"];

	if(fn.empty()){
		cerr << "Cannot find node Points." << endl;
		return false;
	}

	char buf[64];
	pts.resize(numPoints);
	for(int ip = 0; ip < numPoints; ip++){
		snprintf(buf, 63, "Point%05d", ip);
		FileNode fpt = fn[buf];
		if(fpt.empty()){
			cerr << "Cannot find node " << buf << "." << endl;
			return false;
		}
		fpt["x"] >> pts[ip].x;
		fpt["y"] >> pts[ip].y;
		fpt["z"] >> pts[ip].z;
	}

	fn = fs["Edges"];
	if(fn.empty()){
		cerr << "Cannot find node Edges." << endl;
		return false;
	}

	edges.resize(numEdges);
	for(int ie =0; ie < numEdges; ie++){
		snprintf(buf, 63, "Edge%05d", ie);
		FileNode fe = fn[buf];
		if(fe.empty()){
			cerr << "Cannot find node " << buf << "." << endl;
			return false;
		}
		fe["s"] >> edges[ie].s;
		fe["e"] >> edges[ie].e;
	}

	xmin = ymin = zmin = FLT_MAX;
	xmax = ymax = zmax = -FLT_MAX;
	for(int i = 0; i < pts.size(); i++){
		xmin = min(xmin, pts[i].x);
		xmax = max(xmax, pts[i].x);

		ymin = min(ymin, pts[i].y);
		ymax = max(ymax, pts[i].y);

		zmin = min(zmin, pts[i].z);
		zmax = max(ymax, pts[i].z);
	}
	return true;
}

s_obj * s_model::instObj(Mat & camint, Mat & camdist, const double width, const double height)
{
	// instantiate object with the model
	s_obj * pobj = new s_obj(this, camint, camdist, width, height);
	return pobj;
}

//////////////////////////////////////////////////////////////////////////// s_obj member
s_obj::s_obj(s_model * apmdl, const Mat & camint, const Mat & camdist,
	const double width, const double height):pmdl(apmdl)
{
	double xsize = pmdl->get_xsize();
	double ysize = pmdl->get_ysize();
	double zsize = pmdl->get_zsize();

	// To fit inside the window, z should be determined 
	// to satisfy both fpix * xsize / z < width and  fpix * ysize / z < height
	// This means z > fpix * xsize / width and z > fpix * ysize / height.
	// Actually the z should be
	double fpix_x = camint.at<double>(0, 0);
	double fpix_y = camint.at<double>(1, 1);
	double dist_z = max(fpix_x * xsize / width, fpix_y * ysize / height);

	// now rvec is zero and tvec is (0, 0, dist_z)
	rvec = Mat::zeros(3, 1, CV_64FC1);
	tvec = Mat::zeros(3, 1, CV_64FC1);
	tvec.at<double>(2, 0) = dist_z;

	pt2d.resize(pmdl->pts.size());
	pt2dprj.resize(pmdl->pts.size());
	bvisible.resize(pmdl->pts.size(), false);

	apmdl->ref++;
}

bool s_obj::load(const char * afname, vector<s_model> & mdls)
{
	fname = afname;
	return true;
}

bool s_obj::save(const char * afname)
{
	return true;
}

void s_obj::render(Mat & camint, Mat & camdist, Mat & rvec_cam, Mat & tvec_cam,
	LPDIRECT3DDEVICE9 pd3dev, c_d3d_dynamic_text * ptxt, LPD3DXLINE pline,
	int pttype, int state, int cur_point)
{
	pmdl->proj(pt2dprj, camint, camdist, rvec_cam, tvec_cam, rvec, tvec);
	render_prjpts(*pmdl, pt2dprj, pd3dev, ptxt, pline, pttype, state, cur_point);
}

void s_obj::render_axis(Mat & rvec_cam, Mat & tvec_cam, Mat & cam_int, Mat & cam_dist,
	LPDIRECT3DDEVICE9 pd3dev, LPD3DXLINE pline, int axis)
{
	float fac = (float) pmdl->get_max_dist();

	vector<Point3f> p3d(4, Point3f(0.f, 0.f, 0.f));
	vector<Point2f> p2d;

	p3d[1].x = fac; // pt3d[1] is x axis
	p3d[2].y = fac; // pt3d[2] is y axis
	p3d[3].z = fac; // pt3d[3] is z axis

	Mat rvec_comp, tvec_comp;
	composeRT(rvec_cam, tvec_cam, rvec, tvec, rvec_comp, tvec_comp);
	projectPoints(p3d, rvec_comp, tvec_comp, cam_int, cam_dist, p2d);
	pline->Begin();
	D3DXVECTOR2 v[2];
	D3DCOLOR color;
	int arpha = (axis < 0 ? 255 : 128);

	// origin of the axis
	v[0] = D3DXVECTOR2(p2d[0].x, p2d[0].y);

	// x axis
	v[1] = D3DXVECTOR2(p2d[1].x, p2d[1].y);
	if(axis == 0)
		color = D3DCOLOR_RGBA(255, 0, 0, arpha);
	else
		color = D3DCOLOR_RGBA(255, 0, 0, 255);
	pline->Draw(v, 2, color);

	// y axis
	v[1] = D3DXVECTOR2(p2d[2].x, p2d[2].y);
	if(axis == 1)
		color = D3DCOLOR_RGBA(0, 255, 0, arpha);
	else
		color = D3DCOLOR_RGBA(0, 255, 0, 255);
	pline->Draw(v, 2, color);

	// z axis
	v[1] = D3DXVECTOR2(p2d[3].x, p2d[3].y);
	if(axis == 2)
		color = D3DCOLOR_RGBA(0, 0, 255, arpha);
	else
		color = D3DCOLOR_RGBA(0, 0, 255, 255);
	pline->Draw(v, 2, color);

	pline->End();
}

void s_obj::render_vector(Point3f & vec, 
	Mat & rvec_cam, Mat & tvec_cam, Mat & cam_int, Mat & cam_dist,
	LPDIRECT3DDEVICE9 pd3dev, LPD3DXLINE pline)
{
	// calculating scale factor
	double fac = pmdl->get_max_dist() / sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	vec *= fac;

	// projection 
	vector<Point3f> vec3d(2);
	vector<Point2f> vec2d;
	vec3d[0] = Point3f(0, 0, 0);
	vec3d[1] = vec;
	Mat rvec_comp, tvec_comp;
	composeRT(rvec_cam, tvec_cam, rvec, tvec, rvec_comp, tvec_comp);
	projectPoints(vec3d, rvec_comp, tvec_comp, cam_int, cam_dist, vec2d);
	pline->Begin();
	D3DCOLOR color = D3DCOLOR_RGBA(255, 255, 255, 255);

	D3DXVECTOR2 v[2];
	v[0] = D3DXVECTOR2(vec2d[0].x, vec2d[0].y);
	v[1] = D3DXVECTOR2(vec2d[1].x, vec2d[1].y);
	pline->Draw(v, 2, color);

	// draw cross at the origin 
	xcross(pline, vec2d[0], 3.0, color);
	pline->End();
}

//////////////////////////////////////////////////////////////////// class f_inspector
const char * f_inspector::m_str_op[ESTIMATE+1]
= {"model", "obj", "point", "camera", "estimate"};

const char * f_inspector::m_axis_str[AX_Z + 1] = {
	"x", "y", "z"
};

f_inspector::f_inspector(const char * name):f_ds_window(name), m_pin(NULL), m_timg(-1),
	m_sh(1.0), m_sv(1.0), m_bundistort(false), 
	m_bpttrack(false),/* m_bcbtrack(false), m_bchsbd_found(false),
	m_sz_chsbd(6, 9), m_pitch_chsbd(0.0254f), m_bshow_chsbd(false),
	m_num_chsbds_calib(120),*/
	m_bpose_fixed(true), m_bcampar_fixed(true), m_bcam_tbl_loaded(false),
	m_bload_campar(false), m_bload_campar_tbl(false),
	m_bcalib_use_intrinsic_guess(false),
	m_bcalib_fix_principal_point(false),
	m_bcalib_fix_aspect_ratio(false),
	m_bcalib_zero_tangent_dist(true),
	m_bcalib_fix_k1(true), m_bcalib_fix_k2(true), m_bcalib_fix_k3(true),
	m_bcalib_fix_k4(true), m_bcalib_fix_k5(true), m_bcalib_fix_k6(true),
	m_bcalib_rational_model(false),
	m_badd_model(false),
	m_cur_model(-1), m_cur_obj(-1), 
	m_op(OBJ),
	m_pmesh_chsbd(NULL), m_ptex_chsbd(NULL),
	m_mm(MM_NORMAL), m_axis(AX_X), m_rot_step(1.0), m_trn_step(1.0), m_zoom_step(1.1),
	m_main_offset(0, 0), m_main_scale(1.0),
	m_theta_z_mdl(0.0), m_dist_mdl(0.0)
	
{
	m_fname_model[0] = '\0';
	m_fname_campar[0] = '\0';
	m_fname_campar_tbl[0] = '\0';
//	m_fname_chsbds[0] = '\0';
	m_cam_int = Mat::eye(3, 3, CV_64FC1);
	m_cam_dist = Mat::zeros(1, 8, CV_64FC1);
	m_rvec_cam = Mat::zeros(3, 1, CV_64FC1);
	m_tvec_cam = Mat::zeros(3, 1, CV_64FC1);
	register_fpar("fmodel", m_fname_model, 1024, "File path of 3D model frame.");
	register_fpar("add_model", &m_badd_model, "If the flag is asserted, model is loaded from fmodel");
	register_fpar("ldcp", &m_bload_campar, "Load a camera parameter.");
	register_fpar("ldcptbl", &m_bload_campar_tbl, "Load the table of camera parameters with multiple magnifications."); 
	register_fpar("fcp", m_fname_campar, 1024, "File path of camera parameter.");
	register_fpar("fcptbl", m_fname_campar_tbl, 1024, "File path of table of the camera parameters with multiple magnifications.");
	register_fpar("op", (int*)&m_op, ESTIMATE+1, m_str_op,"Operation ");
	register_fpar("sh", &m_sh, "Horizontal scaling value. Image size is multiplied by the value. (default 1.0)");
	register_fpar("sv", &m_sv, "Vertical scaling value. Image size is multiplied by the value. (default 1.0)");	

	// chessboard related parameters
//	register_fpar("fchsbds", m_fname_chsbds, 1024, "File path of chessboard collections");
//	register_fpar("cbtrack", &m_bcbtrack, "Chessboard tracking enabled.");
//	register_fpar("pcb", &m_pitch_chsbd, "Pitch of the chesboard (0.0254m default)");
//	register_fpar("vcb", &(m_sz_chsbd.height), "Number of vertical grids in the chessboard (6 default)");
//	register_fpar("hcb", &(m_sz_chsbd.width), "Number of horizontal grids in the chessboard (9 default)");
//	register_fpar("showcb", &m_bshow_chsbd, "Show detected chessboard.");
//	register_fpar("cbdet", &m_bchsbd_found, "Chessboard found flag");
// 	register_fpar("chsbds", &m_num_chsbds_calib, "Number of chessboards used for calibration.");

	// model related parameters
	register_fpar("pttrack", &m_bpttrack, "Model point tracking enabled.");
	register_fpar("mdl", &m_cur_model, "Model with specified index is selected.");
	register_fpar("pt", &m_cur_point, "Model point of specified index is selected.");

	// camera calibration and parameter
	register_fpar("fx", m_cam_int.ptr<double>(0, 0), "x-directional focal length in milimeter");
	register_fpar("fy", m_cam_int.ptr<double>(1, 1), "y-directional focal length in milimeter");
	register_fpar("cx", m_cam_int.ptr<double>(0, 2), "x-coordinate of camera center in pixel.");
	register_fpar("cy", m_cam_int.ptr<double>(1, 2), "y-coordinate of camera center in pixel.");
	register_fpar("px", m_cam_dist.ptr<double>(0) + 2, "x coefficient of tangential distortion.");
	register_fpar("py", m_cam_dist.ptr<double>(0) + 3, "y coefficient of tangential distortion.");
	register_fpar("k1", m_cam_dist.ptr<double>(0), "Radial distortion coefficient k1.");
	register_fpar("k2", m_cam_dist.ptr<double>(0) + 1, "Radial distortion coefficient k2.");
	register_fpar("k3", m_cam_dist.ptr<double>(0) + 4, "Radial distortion coefficient k3.");
	register_fpar("k4", m_cam_dist.ptr<double>(0) + 5, "Radial distortion coefficient k4.");
	register_fpar("k5", m_cam_dist.ptr<double>(0) + 6, "Radial distortion coefficient k5.");
	register_fpar("k6", m_cam_dist.ptr<double>(0) + 7, "Radial distortion coefficient k6.");
	register_fpar("erep", &m_erep, "Reprojection error.");

	register_fpar("use_intrinsic_guess", &m_bcalib_use_intrinsic_guess, "Use intrinsic guess.");
	register_fpar("fix_principal_point", &m_bcalib_fix_principal_point, "Fix camera center as specified (cx, cy)");
	register_fpar("fix_aspect_ratio", &m_bcalib_fix_aspect_ratio, "Fix aspect ratio as specified fx/fy. Only fy is optimized.");
	register_fpar("zero_tangent_dist", &m_bcalib_zero_tangent_dist, "Zeroify tangential distortion (px, py)");
	register_fpar("fix_k1", &m_bcalib_fix_k1, "Fix k1 as specified.");
	register_fpar("fix_k2", &m_bcalib_fix_k2, "Fix k2 as specified.");
	register_fpar("fix_k3", &m_bcalib_fix_k3, "Fix k3 as specified.");
	register_fpar("fix_k4", &m_bcalib_fix_k4, "Fix k4 as specified.");
	register_fpar("fix_k5", &m_bcalib_fix_k5, "Fix k5 as specified.");
	register_fpar("fix_k6", &m_bcalib_fix_k6, "Fix k6 as specified.");
	register_fpar("rational_model", &m_bcalib_rational_model, "Enable rational model (k4, k5, k6)");

	register_fpar("undist", &m_bundistort, "Undistort source image according to the camera parameter.");

	// state flag
	register_fpar("cpfix", &m_bcampar_fixed, "Camera parameter fixed flag");

	// object/camera manipulation
	register_fpar("axis", (int*)&m_axis, (int)AX_Z + 1, m_axis_str, "Axis for rotation and translation. {x, y, z}");
	register_fpar("srot", &m_rot_step, "Rotation step for the camera and objects. (degree)");
	register_fpar("strn", &m_trn_step, "Translation step for the camera and objects. (meter)");
	register_fpar("szm", &m_zoom_step, "Zooming step for the camera and screen. (default 1.1)");
}

f_inspector::~f_inspector()
{
}

bool f_inspector::alloc_d3dres()
{
	if(!f_ds_window::alloc_d3dres()){
		return false;
	}
	if(!m_model_view.init(m_pd3dev,
		(float) m_ViewPort.Width, (float) m_ViewPort.Height, 
		(float) m_ViewPort.Width, (float) m_ViewPort.Height, 
		(float) m_ViewPort.Width, (float) m_ViewPort.Height))
		return false;

	return true;
}

void f_inspector::release_d3dres()
{
	f_ds_window::release_d3dres();

	m_model_view.release();
	return;
}

bool f_inspector::proc()
{
	pthread_lock lock(&m_d3d_mtx);

	////////////////// updating pvt information ///////////////////////
	long long timg;	
	Mat img;
	if(!is_pause()){
		img = m_pin->get_img(timg);
		if(img.empty())
			return true;

		if(m_timg != timg){
//			m_bchsbd_found = false;
			m_bpose_fixed = false;
		}
		m_timg = timg;
		m_img = img;
	}else{
		timg = m_timg;
		img = m_img;
	}

	// input source is not ready. but it tends to happen usually.
	if(img.empty())
		return true;

	Mat img_s;
	resize(img, img_s, Size(), m_sh, m_sv);

	// fit the viewport size to the image
	if(img_s.cols != m_ViewPort.Width ||
		img_s.rows != m_ViewPort.Height){
		if(!init_viewport(img_s)){
			return false;
		}
	}

	// fit the direct 3d surface to the image
	if(img_s.cols != m_maincam.get_surface_width() ||
		img_s.rows != m_maincam.get_surface_height())
	{
		m_maincam.release();
		if(!m_maincam.init(m_pd3dev,  
			(float) img_s.cols, (float) img_s.rows, 
			(float) m_ViewPort.Width, (float) m_ViewPort.Height, 
			(float) m_ViewPort.Width, (float) m_ViewPort.Height))
			return false;
	}

	// fit the direct 3d surface of the model view to the image
	if((img_s.cols != m_model_view.get_surface_width() || 
		img_s.rows != m_model_view.get_surface_height()))
	{
		m_model_view.release();
		if(!m_model_view.init(m_pd3dev,
			(float) img_s.cols, (float) img_s.rows,
			(float) m_ViewPort.Width, (float) m_ViewPort.Height,
			(float) m_ViewPort.Width, (float) m_ViewPort.Height))
			return false;
	}

	//////////////// Model related code //////////////////////////////
	if(m_badd_model){
		if(!load_model()){
			cerr << "Failed to load model " << m_fname_model << endl;
		}
		m_badd_model = false;
	}

	//////////////// Chessboard related code /////////////////////////
	/*
	switch(m_op){
	case DET_CHSBD:
		if(!m_bchsbd_found)
			findChsbd(img_s, timg);
		break;
	case SAVE_CHSBDS:
		if(!saveChsbds()){
			cerr << "Failed to save Chess boards." << endl;
		}else{
			cout << "Chessboards successfully saved." << endl;
		}
		m_op = NORMAL;
		break;
	case LOAD_CHSBDS:
		if(!loadChsbds()){
			cerr << "Failed to load Chessboards." << endl;
		}else{
			cout << "Chessboards successfully loaded." << endl;
		}
		m_op = NORMAL;
		break;
	case CLEAR_CHSBDS:
		clearChsbds();
		break;
	}
	*/

	// calibration
	//calibrate(img_s, timg);
	
	// rendering main view
	render(img_s, timg);

	return true;
}

/* now chessboard handling codes are unified into object handling codes
void f_inspector::initChsbd3D()
{
	HRESULT hr;
	m_3dchsbd.resize(m_sz_chsbd.height*m_sz_chsbd.width);
	for(int i= 0; i < m_sz_chsbd.height; i++){
		for(int j = 0; j < m_sz_chsbd.width; j++){
			int ipt = m_sz_chsbd.width * i + j;
			m_3dchsbd[ipt].x = (float) (m_pitch_chsbd * i);
			m_3dchsbd[ipt].y = (float)(m_pitch_chsbd * j);
			m_3dchsbd[ipt].z = 0.f;
		}
	}

	// creating chessboard model
	D3DXCreateMeshFVF(2, 4, D3DXMESH_MANAGED, 
		ModelVertex::FVF, m_pd3dev, &m_pmesh_chsbd);
	// set vertex buffer
	ModelVertex * v;
	m_pmesh_chsbd->LockVertexBuffer(0, (void**) &v);
	v[0] = ModelVertex((float)(-m_pitch_chsbd), (float)(-m_pitch_chsbd), 0.f, 
		0.f, 0.f, 1.f,
		0.f, 0.f);
	v[1] = ModelVertex((float)(m_pitch_chsbd * 9), (float)(-m_pitch_chsbd), 0.f, 
		0.f, 0.f, 1.f, 
		1.f, 0.f);
	v[2] = ModelVertex((float)(m_pitch_chsbd * 9), (float)(m_pitch_chsbd * 6), 0.f, 
		0.f, 0.f, 0.f, 
		1.f, 1.f);
	v[3] = ModelVertex((float)(-m_pitch_chsbd), (float)(m_pitch_chsbd * 6), 0.f, 
		0.f, 0.f, 0.f, 
		0.f, 1.f);
	m_pmesh_chsbd->UnlockVertexBuffer();

	// set index buffer
	WORD * i;
	m_pmesh_chsbd->LockIndexBuffer(0, (void**) &i);
	// Direct3D assumes counter clockwise vertex ordering in culling.
	// however, we flip the coordinate from right-hand to left-hand 
	// during view transformation. so the vertex ordering is now in clockwise
	// ordering.
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;
	m_pmesh_chsbd->UnlockIndexBuffer();

	// set attribute buffer
	DWORD * a;
	m_pmesh_chsbd->LockAttributeBuffer(0, (DWORD**) &a);
	a[0] = 0;
	a[1] = 0;
	m_pmesh_chsbd->UnlockAttributeBuffer();

	// 
	vector<DWORD> ajbuf(m_pmesh_chsbd->GetNumFaces() * 3);
	m_pmesh_chsbd->GenerateAdjacency(0.f, &ajbuf[0]);
	m_pmesh_chsbd->OptimizeInplace(
		D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_COMPACT | D3DXMESHOPT_VERTEXCACHE,           
		&ajbuf[0], 0, 0, 0);   

	// set texture buffer	
	hr = D3DXCreateTextureFromFile(m_pd3dev, 
		_T("7x10calib-checkerboard_trimed.png"), &m_ptex_chsbd);

	if(hr != D3D_OK){
		cerr << "Failed to load Chessboard texture" << endl;
		m_ptex_chsbd = NULL;
	}

	m_cur_chsbd = 0;
}

void f_inspector::seekChsbdTime(long long timg)
{
	m_bchsbd_found = false;
	// seraching for current chessboard
	for(; m_cur_chsbd < m_2dchsbd.size(); m_cur_chsbd++){
		if(m_time_chsbd[m_cur_chsbd] == timg){
			m_bchsbd_found = true;
			cout << "Chessboard found in stock" << endl;
			break;
		}

		if(m_time_chsbd[m_cur_chsbd] > timg){
			break;
		}
	}
}

void f_inspector::findChsbd(Mat & img, long long timg)
{
	// seraching for current chessboard
	seekChsbdTime(timg);
	if(m_bchsbd_found)
		return;

	m_corners_chsbd.clear();
	Mat gry;
	cvtColor(img, gry, CV_RGB2GRAY);
	if(m_bchsbd_found = findChessboardCorners(gry, m_sz_chsbd, m_corners_chsbd)){
		cornerSubPix(gry, m_corners_chsbd, Size(11, 11), Size(-1, -1),
			TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
		m_2dchsbd.insert(m_2dchsbd.begin() + m_cur_chsbd, m_corners_chsbd);
		m_time_chsbd.insert(m_time_chsbd.begin() + m_cur_chsbd, m_timg);
		m_rvecs_chsbd.insert(m_rvecs_chsbd.begin() + m_cur_chsbd, Mat(3, 1, CV_64FC1));
		m_tvecs_chsbd.insert(m_tvecs_chsbd.begin() + m_cur_chsbd, Mat(3, 1, CV_64FC1));
		cout << "Chessboard newly added." << endl;
		cout << m_time_chsbd.size() << " chessboard stocked." << endl;
	}else{
		cerr <<  "Failed to find chessboard." << endl;
	}
	m_bcampar_fixed = false;
}

void f_inspector::clearChsbds()
{
	m_2dchsbd.clear();
	m_time_chsbd.clear();
	m_bchsbd_found = false;
}

bool f_inspector::saveChsbds()
{
	FileStorage fs(m_fname_chsbds, FileStorage::WRITE);
	if(!fs.isOpened()){
		return false;
	}

	int num_chsbds = (int) m_2dchsbd.size();
	int num_pts = m_sz_chsbd.width * m_sz_chsbd.height;
	fs << "ChsbdSize" << m_sz_chsbd;
	fs << "ChsbdPitch" << m_pitch_chsbd;
	fs << "NumChsbds" << num_chsbds;
	fs << "Chsbds" << "{";
	for(int icb = 0; icb < num_chsbds; icb++){
		char buf[128];
		snprintf(buf, 128, "Chsbd%04d", icb);
		fs << buf << "{";
		long long t = m_time_chsbd[icb];
		fs << "TimeU" <<  *((int*) &t + 1);
		fs << "TimeL" << *((int*) &t);
		if(m_bchsbd_pose_fixed.size() == num_chsbds && 
			m_bchsbd_pose_fixed[icb]){
				fs << "Err" << m_ereps_chsbd[icb];
				fs << "Pose" << m_bchsbd_pose_fixed[icb];
				fs << "Rvec" << m_rvecs_chsbd[icb];
				fs << "Tvec" << m_tvecs_chsbd[icb];
		}
		fs << "Pts" << "[";
		vector<Point2f> & chsbd2d = m_2dchsbd[icb];
		for(int ipt = 0; ipt < num_pts; ipt++){
			fs << chsbd2d[ipt];
		}
		fs << "]";
		fs << "}";
	}
	fs << "}";

	return true;
}

bool f_inspector::loadChsbds()
{
	clearChsbds();

	FileStorage fs(m_fname_chsbds, FileStorage::READ);
	if(!fs.isOpened()){
		return false;
	}

	FileNode fn;

	fn = fs["ChsbdSize"];

	if(fn.empty()){
		return false;
	}
	fn >> m_sz_chsbd;

	int num_pts = m_sz_chsbd.width * m_sz_chsbd.height;
	initChsbd3D();

	fn = fs["ChsbdPitch"];

	if(fn.empty()){
		return false;
	}

	fn >> m_pitch_chsbd;

	int num_chsbds;

	fn = fs["NumChsbds"];

	if(fn.empty()){
		return true;
	}

	fn >> num_chsbds;

	m_2dchsbd.resize(num_chsbds);
	m_time_chsbd.resize(num_chsbds);
	m_rvecs_chsbd.resize(num_chsbds);
	m_tvecs_chsbd.resize(num_chsbds);
	m_ereps_chsbd.resize(num_chsbds, 0.);
	m_bchsbd_pose_fixed.resize(num_chsbds, false);

	fn = fs["Chsbds"];
	if(fn.empty()){
		return false;
	}

	for(int icb = 0; icb < num_chsbds; icb++){
		m_2dchsbd[icb].resize(num_pts);
		char buf[128];
		snprintf(buf, 128, "Chsbd%04d", icb);
		FileNode fnchsbd = fn[buf];
		FileNode fnsub;

		if(fnchsbd.empty())
			return false;

		fnsub = fnchsbd["TimeU"];
		long long t;
		if(fnsub.empty())
			return false;
		fnsub >> *((int*) &t + 1);

		fnsub = fnchsbd["TimeL"];
		if(fnsub.empty())
			return false;
		fnsub >> *((int*) &t);
		m_time_chsbd[icb] = t;

		fnsub = fnchsbd["Err"];
		if(!fnsub.empty()){
			fnsub >> m_ereps_chsbd[icb];
			m_bchsbd_pose_fixed[icb] = true;
		}

		fnsub = fnchsbd["Rvec"];
		if(!fnsub.empty())
			fnsub >> m_rvecs_chsbd[icb];

		fnsub = fnchsbd["Tvec"];
		if(!fnsub.empty())
			fnsub >> m_tvecs_chsbd[icb];

		fnsub = fnchsbd["Pts"];
		if(fnsub.empty())
			return false;

		FileNodeIterator itr_pt = fnsub.begin();
		vector<Point2f> & chsbd2d = m_2dchsbd[icb];
		for(int ipt = 0; ipt < num_pts; ipt++, itr_pt++){
			if((*itr_pt).empty())
				return false;
			(*itr_pt) >> chsbd2d[ipt];			
		}
	}

	return true;
}


bool f_inspector::chooseChsbds(vector<vector<Point2f > > & chsbd2d, vector<int> & id_chsbd)
{
	if(m_2dchsbd.size() < m_num_chsbds_calib)
		return false;

	int step = (int) ((double) m_2dchsbd.size() / (double) m_num_chsbds_calib);
	for(int i = 0, id = 0; i < m_num_chsbds_calib; i++, id += step){
		chsbd2d.push_back(m_2dchsbd[id]);
		id_chsbd.push_back(id);
	}

	return true;
}

void f_inspector::calibChsbd(Mat & img)
{
	vector<vector<Point3f > > chsbds3d;
	vector<vector<Point2f > > chsbds2d;
	vector<int> id_chsbd;
	vector<Mat> tvecs, rvecs;

	// initializing resulting data structure
	m_rvecs_chsbd.clear();
	m_tvecs_chsbd.clear();
	m_bchsbd_pose_fixed.clear();
	m_ereps_chsbd.clear();

	m_rvecs_chsbd.resize(m_2dchsbd.size());
	m_tvecs_chsbd.resize(m_2dchsbd.size());
	m_bchsbd_pose_fixed.resize(m_2dchsbd.size(), false);
	m_ereps_chsbd.resize(m_2dchsbd.size(), DBL_MAX);

	cout << "Selecting " << m_num_chsbds_calib << " chessboards." << endl;
	if(!chooseChsbds(chsbds2d, id_chsbd))
		return;

	int flag = 0;
	flag |= (m_bcalib_use_intrinsic_guess ? CV_CALIB_USE_INTRINSIC_GUESS : 0);
	flag |= (m_bcalib_fix_principal_point ? CV_CALIB_FIX_PRINCIPAL_POINT : 0);
	flag |= (m_bcalib_fix_aspect_ratio ? CV_CALIB_FIX_ASPECT_RATIO : 0);
	flag |= (m_bcalib_zero_tangent_dist ? CV_CALIB_ZERO_TANGENT_DIST : 0);
	flag |= (m_bcalib_fix_k1 ? CV_CALIB_FIX_K1 : 0);
	flag |= (m_bcalib_fix_k2 ? CV_CALIB_FIX_K2 : 0);
	flag |= (m_bcalib_fix_k3 ? CV_CALIB_FIX_K3 : 0);
	flag |= (m_bcalib_fix_k4 ? CV_CALIB_FIX_K4 : 0);
	flag |= (m_bcalib_fix_k5 ? CV_CALIB_FIX_K5 : 0);
	flag |= (m_bcalib_fix_k6 ? CV_CALIB_FIX_K6 : 0);
	flag |= (m_bcalib_rational_model ? CV_CALIB_RATIONAL_MODEL : 0);

	for(int i = 0; i < chsbds2d.size(); i++)
		chsbds3d.push_back(m_3dchsbd);

	Mat Cdist;
	Cdist = m_cam_dist.clone();
	m_erep = calibrateCamera(chsbds3d, chsbds2d, 
		Size(img.cols, img.rows), m_cam_int, Cdist,
		rvecs, tvecs, flag);

	for(int i = 0; i < chsbds2d.size(); i++){
		int id = id_chsbd[i];
		m_rvecs_chsbd[id] = rvecs[i];
		m_tvecs_chsbd[id] = tvecs[i];
		m_bchsbd_pose_fixed[id] = true;
	}

	MatIterator_<double> src_itr, src_end;
	MatIterator_<double> dst_itr;

	for(src_itr = Cdist.begin<double>(), dst_itr = m_cam_dist.begin<double>(),
		src_end = Cdist.end<double>();
		src_itr != src_end; src_itr++, dst_itr++)
		*dst_itr = *src_itr;

	cout << "Calibration done with reprojection error of " << m_erep << endl;
	cout << "Mcam = " << m_cam_int << endl;
	cout << "Cdist = " << m_cam_dist << endl;

	// calculate reprojection error
	m_ereps_chsbd.resize(m_2dchsbd.size());
	m_bchsbd_pose_fixed.resize(m_2dchsbd.size());
	for(int icb = 0; icb < m_2dchsbd.size(); icb++){
		if(!m_bchsbd_pose_fixed[icb])
			continue;

		m_ereps_chsbd[icb] = 0.;
		vector<Point2f> impts;
		vector<Point2f> & chsbd2d = m_2dchsbd[icb];
		projectPoints(m_3dchsbd, m_rvecs_chsbd[icb], m_tvecs_chsbd[icb],
			m_cam_int, m_cam_dist, impts);
		for(int ipt = 0; ipt < impts.size(); ipt++){
			Point2f pd, pp;
			pd = impts[ipt];
			pp = chsbd2d[ipt];
			double dx = pp.x - pd.x;
			double dy = pp.y - pd.y;
			m_ereps_chsbd[icb] += dx * dx + dy * dy;
		}
		m_ereps_chsbd[icb] = sqrt(m_ereps_chsbd[icb] / (double) impts.size());
		m_bchsbd_pose_fixed[icb] = true;
	}

	// inserting to the table. Table is sorted in descending order for focal length.
	vector<Mat>::iterator itr_int = m_cam_int_tbl.begin();
	vector<Mat>::iterator itr_int_end = m_cam_int_tbl.end();
	vector<Mat>::iterator itr_dist = m_cam_dist_tbl.begin();
	vector<double>::iterator itr_erep = m_cam_erep.begin();
	for(;itr_int != itr_int_end; itr_int++, itr_dist++, itr_erep++){
		double fx_0 = (*itr_int).at<double>(0, 0);
		double fx_1 = m_cam_int.at<double>(0, 0);
		if(fx_1 < fx_0){
			break;
		}
	}
	m_cam_int_tbl.insert(itr_int, m_cam_int.clone());
	m_cam_dist_tbl.insert(itr_dist, m_cam_dist.clone());
	m_cam_erep.insert(itr_erep, m_erep);

	m_bcampar_fixed = true;
}

void f_inspector::guessCamparPauseChsbd(long long timg)
{
	int num_models = 1;
	seekChsbdTime(timg);
	vector<int> num_pts;
	num_pts.push_back((int)m_3dchsbd.size());
	vector<vector<Point3f> > p3d;
	p3d.push_back(m_3dchsbd);
	vector<vector<Point2f> > p2d;
	p2d.push_back(m_2dchsbd[m_cur_chsbd]);

	if(m_cam_int.cols != 3 || m_cam_int.rows != 3 || m_cam_int.type() != CV_64FC1){
		m_cam_int = Mat::eye(3, 3, CV_64FC1);
		m_cam_dist = Mat::zeros(8, 1, CV_64FC1);
	}

	s_package pkg(num_models, num_pts, p2d, p3d,
		m_cam_int_tbl, m_cam_dist_tbl, 
		m_cam_int, m_cam_dist);

	int m = pkg.num_models * 6; // rvecs and tvecs (6degree of freedom for each model.)
	switch(m_op){
	case DET_POSE_CAM:
		m += 11;
		break;
	case DET_POSE_CAM_TBL:
		m += 1;
		if(!m_bcam_tbl_loaded){
			cerr << "Estimation using camera parameter table requires the table should be loaded." << endl;
			return;
		}
		break;
	case DET_POSE:
		if(!m_bcampar_fixed){
			cerr << "Estimating only pose requires camera intrinsics to be fixed" << endl;
			return;
		}
		break;
	}

	c_aws_lmdif lm((int)(p2d.size() * 2), m);
	int info;
	switch(m_op){
	case DET_POSE_CAM:
		info = lm.optimize(prj_pause_and_cam, (void*)&pkg, 1.);
		m_cam_int.at<double>(0, 0) = lm.x(0);
		m_cam_int.at<double>(1, 1) = lm.x(1);
		m_cam_int.at<double>(0, 2) = lm.x(2);
		m_cam_int.at<double>(1, 2) = lm.x(3);
		Mat(8, 1, CV_64FC1, (void*) &lm.x(4)).copyTo(m_cam_dist);
		Mat(3, 1, CV_64FC1, (void*) &lm.x(12)).copyTo(m_rvecs_chsbd[m_cur_chsbd]);
		Mat(3, 1, CV_64FC1, (void*) &lm.x(15)).copyTo(m_tvecs_chsbd[m_cur_chsbd]);
		break;
	case DET_POSE_CAM_TBL:
		info = lm.optimize(prj_pause_and_cam_with_tbl, (void*)&pkg, 1.);
		m_cam_int_tbl[(int)(lm.x(0) + 0.5)].copyTo(m_cam_int);
		m_cam_dist_tbl[(int)(lm.x(0) + 0.5)].copyTo(m_cam_dist);
		Mat(3, 1, CV_64FC1, (void*) &lm.x(1)).copyTo(m_rvecs_chsbd[m_cur_chsbd]);
		Mat(3, 1, CV_64FC1, (void*) &lm.x(4)).copyTo(m_tvecs_chsbd[m_cur_chsbd]);
		break;
	case DET_POSE:
		info = lm.optimize(prj_pause, (void*)&pkg, 1.);
		Mat(3, 1, CV_64FC1, (void*) &lm.x(0)).copyTo(m_rvecs_chsbd[m_cur_chsbd]);
		Mat(3, 1, CV_64FC1, (void*) &lm.x(3)).copyTo(m_tvecs_chsbd[m_cur_chsbd]);
		break;
	}

	if(info < 4 && info > 0)
		m_bcampar_fixed = true;

	switch(info){
	case 0:
		cout << "Improper input parameters" << endl;
		break;
	case 1:
		cout << "Sum of error squares is at most tol" << endl;
		break;
	case 2:
		cout << "Error in solution is at most tol" << endl;
		break;
	case 3:
		cout << "Both sum of error squares and error in solution is at most tol" << endl;
		break;
	case 4:
		cout << "fvec is orthogonal to the column of the Jacobian." << endl;
		break;
	case 5:
		cout << "Number of iterations exceeds " << 200 * (m + 1) << endl; 
		break;
	case 6:
		cout << "Tol is too small for sum of error squares." << endl;
		break;
	case 7:
		cout << "Tol is too small for error in solution." << endl;
		break;
	}

	// store results

	// constants passed as p
	// camint: 
	// n : the number of models
	// m_1, ..., m_n: the number of corresponding points
	// 2D-3D model points
	// (x11, y11)-(X11, Y11, Z11), ...., (x1m_1,y1m_1)-(X1m_1, Y1m_1, Z1m_1)
	// ...
	// (xn1, yn1)-(Xn1, Yn1, Zn1), ....., (xnm_n, ynm_n)-(Xnm_n, Ynm_n, Znm_n)

	// 1. optimize campar and pause
	// parameters optimized
	// fx,fy,cx,cy,px,py,k1, ,,, k6, 
	// rvec_1, tvec_1, rvec2, tvec2, ..., rvec_n, tvec_n
	// 2. optimize campar and pause using table
	// parameters optimized
	// f_pos
	// rvec_1, tvec_1, rvec2, tvec2, ..., rvec_n, tvec_n
	// 3. Optimize pause using fixed intrinsic campar
}

void f_inspector::calibrate(Mat & img_s, long long timg)
{
	switch(m_op){
	case CALIB:
		if(!m_bcampar_fixed)
			calibChsbd(img_s);
		break;
	case SAVE_CAMPAR:
		if(!saveCampar())
			cerr << "Failed to save camera parameter" << endl;
		m_op = NORMAL;
		break;
	case LOAD_CAMPAR:
		if(!loadCampar())
			cerr << "Failed to load camera parameter" << endl;
		m_op = NORMAL;
		break;
	case CLEAR_CAMPAR:
		clearCampar();
		m_op = NORMAL;
		break;
	case DET_POSE:
	case DET_POSE_CAM:
	case DET_POSE_CAM_TBL:
		guessCamparPauseChsbd(timg);
		guessCamparPauseModel(timg);
		m_op = NORMAL;
	}
}


void f_inspector::renderChsbd(long long timg)
{	
	D3DXMATRIX Mtrn;

	// if chesbord is not there, return without rendering
	seekChsbdTime(timg);

	// chess board is found 
	if(!m_bchsbd_found)
		return;

	// the camera parameter is fixed
	if(!m_bcampar_fixed)
		return;

	// the chesboard pose is fixed
	if(m_rvecs_chsbd.size() <= m_cur_chsbd)
		return;

	//		for(int icb = 0; icb < m_2dchsbd.size(); icb++){
	int icb = m_cur_chsbd;
	m_pd3dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);   
	m_pd3dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);   
	m_pd3dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);      
	m_pd3dev->SetRenderState(D3DRS_LIGHTING, false);   
	m_pd3dev->SetTexture(0, m_ptex_chsbd);

	Mat tvec = m_tvecs_chsbd[icb];
	Mat rmat;
	Rodrigues(m_rvecs_chsbd[icb], rmat);
	double * prot = rmat.ptr<double>(0);
	double * ptvec = tvec.ptr<double>(0);
	Mtrn(0, 0) = (float) prot[0];
	Mtrn(1, 0) = (float) prot[1];
	Mtrn(2, 0) = (float) prot[2];
	Mtrn(3, 0) = (float) ptvec[0];

	Mtrn(0, 1) = (float) prot[3];
	Mtrn(1, 1) = (float) prot[4];
	Mtrn(2, 1) = (float) prot[5];
	Mtrn(3, 1) = (float) ptvec[1];

	Mtrn(0, 2) = (float) prot[6];
	Mtrn(1, 2) = (float) prot[7];
	Mtrn(2, 2) = (float) prot[8];
	Mtrn(3, 2) = (float) ptvec[2];

	Mtrn(0, 3) = 0.;
	Mtrn(1, 3) = 0.;
	Mtrn(2, 3) = 0.;
	Mtrn(3, 3) = 1.0;
	m_pd3dev->SetTransform(D3DTS_WORLD, &Mtrn);
	m_pmesh_chsbd->DrawSubset(0);
}

*/

bool f_inspector::saveCampar()
{
	FileStorage fs(m_fname_campar, FileStorage::WRITE);
	if(!fs.isOpened())
		return false;

	fs << "CamPar" << "{";
	fs << "Int" << m_cam_int;
	fs << "Dist" << m_cam_dist;
	fs << "Err" << m_erep;
	return true;
}

bool f_inspector::saveCamparTbl()
{
	FileStorage fs(m_fname_campar_tbl, FileStorage::WRITE);
	if(!fs.isOpened())
		return false;

	fs << "CamPars" << (int) m_cam_int_tbl.size();
	fs << "Pars" << "{";
	for(int ipar = 0; ipar < m_cam_int_tbl.size(); ipar++){
		char buf[128];
		snprintf(buf, 128, "Par%03d", ipar);
		fs << buf << "{";
		fs << "Int" << m_cam_int_tbl[ipar];
		fs << "Dist" << m_cam_dist_tbl[ipar];
		fs << "Err" << m_cam_erep[ipar];
		fs << "}";
	}

	fs << "}";
	return true;
}

bool f_inspector::loadCampar()
{
	clearCampar();

	FileStorage fs(m_fname_campar, FileStorage::READ);
	if(!fs.isOpened())
		return false;
	FileNode fn = fs["CamPar"];
	if(fn.empty())
		return false;

	FileNode fnsub;
	fnsub = fn["Int"];
	if(fnsub.empty())
		return false;
	fnsub >> m_cam_int;

	fnsub = fn["Dist"];
	if(fnsub.empty())
		return false;
	fnsub >> m_cam_dist;

	fnsub = fn["Err"];
	if(!fnsub.empty())
		fnsub >> m_erep;
	return true;
}

bool f_inspector::loadCamparTbl()
{
	clearCamparTbl();

	FileStorage fs(m_fname_campar_tbl, FileStorage::READ);
	if(!fs.isOpened())
		return false;

	int num_pars;
	FileNode fn = fs["CamPars"];
	if(fn.empty())
		return false;

	fn >> num_pars;
	m_cam_int_tbl.resize(num_pars);
	m_cam_dist_tbl.resize(num_pars);
	m_cam_erep.resize(num_pars);

	fn = fs["Pars"];
	if(fn.empty())
		return false;

	FileNodeIterator itr = fn.begin();
	for(int ipar = 0; ipar < num_pars; ipar++, itr++){
		char buf[128];
		snprintf(buf, 128, "Par%03d", ipar);
		FileNode fnpar = fn[buf];
		FileNode fnsub;

		if(fnpar.empty())
			return false;

		fnsub = fnpar["Int"];
		if(fnsub.empty())
			return false;
		fnsub >> m_cam_int_tbl[ipar];

		fnsub = fnpar["Dist"];
		if(fnsub.empty())
			return false;
		fnsub >> m_cam_dist_tbl[ipar];

		fnsub = fnpar["Err"];
		if(fnsub.empty())
			continue;
		fnsub >> m_cam_erep[ipar];
	}

	m_bcam_tbl_loaded = true;
	m_bcampar_fixed = false;
	return true;
}

void f_inspector::clearCampar()
{
	m_cam_int.release();
	m_cam_dist.release();
	m_erep = 0.0;
	m_bcampar_fixed = false;
}

void f_inspector::clearCamparTbl()
{
	m_cam_int_tbl.clear();
	m_cam_dist_tbl.clear();
	m_cam_erep.clear();

	m_bcam_tbl_loaded = false;
	m_bcampar_fixed = false;
}

bool f_inspector::load_model()
{
	s_model mdl;
	if(mdl.load(m_fname_model)){
		m_models.push_back(mdl);
		m_cur_model = (int)(m_models.size() - 1);
		return true;
	}
	return false;
}

void f_inspector::load_obj()
{	
	vector<s_obj>::iterator itr =  m_obj.begin();
	for(;itr != m_obj.end(); itr++){
		if(itr->fname == m_fname_obj){
			return;
		}
	}

	m_obj.push_back(s_obj());
	itr = m_obj.end() - 1;
	if(!itr->load(m_fname_obj, m_models)){
		m_obj.pop_back();
	}
}

void f_inspector::save_obj()
{
	if(m_cur_obj < 0){
		return;
	}
	m_obj[m_cur_obj].save(m_fname_obj);
}

void f_inspector::render3D(long long timg)
{
	m_model_view.SetAsRenderTarget(m_pd3dev);
	m_pd3dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_COLORVALUE(0.f, 0.f, 0.f, 1.0f), 1.0f, 0);
	// rendering chessboard
	D3DXMATRIX Mprj, Mviewcam, Mview;
	D3DXMatrixIdentity(&Mprj);
	double w, h;
	w = (double) m_model_view.get_surface_width();
	h = (double) m_model_view.get_surface_height();

	float zn = 0.1f, zf = 100.f;
	float q = (float)(zf / (zf - zn));
	float fx = (float) m_cam_int.at<double>(0, 0);
	float fy = (float) m_cam_int.at<double>(1, 1);
	float cx = (float) m_cam_int.at<double>(0, 2);
	float cy = (float) m_cam_int.at<double>(1, 2);
	float nfacx = (float) (0.5 * w);
	float nfacy = (float) (0.5 * h);

	// focal length (normalized with screen size)
	Mprj(0, 0) = (float) (fx / nfacx);
	Mprj(1, 1) = (float) (fy / nfacy);

	// principal point (normalized with screen size)
	Mprj(2, 0) = (float) (cx / (double) (0.5 * w) - 1.0); 
	Mprj(2, 1) = (float) (1.0 - cy / (double) (0.5 * h));

	Mprj(2, 2) =  q;
	Mprj(3, 2) = (float) (- q * zn);
	Mprj(2, 3) = 1.0; // original z is reserved as forth dimension.
	m_pd3dev->SetTransform(D3DTS_PROJECTION, &Mprj);

	// setting view matrix
	D3DXMatrixIdentity(&Mviewcam);
	// because we assume righthand coordinate for world space.
	// but the camera coordinate is the lefthand one.
	// we need to flip y-axis, and also need to be careful about culling direction.
	Mviewcam(1, 0) = -Mviewcam(1, 0); 
	Mviewcam(1, 1) = -Mviewcam(1, 1); 
	Mviewcam(1, 2) = -Mviewcam(1, 2); 
	Mviewcam(1, 3) = -Mviewcam(1, 3); 

	m_pd3dev->SetTransform(D3DTS_VIEW, &Mviewcam);

	// rendering chessboard
//	renderChsbd(timg);

	// rendering 3d mdoel
	renderModel(timg);

	m_model_view.ResetRenderTarget(m_pd3dev);
}

//////////////////////////////////////////////// renderer
void f_inspector::render(Mat & imgs, long long timg)
{
	// Image level rendering
	/*
	if(m_bshow_chsbd && m_bchsbd_found){
		drawChessboardCorners(imgs, m_sz_chsbd, 
			m_2dchsbd[m_cur_chsbd], m_bchsbd_found);
	}
	*/

	// undistort if the flag is enabled.
	if(m_bundistort){
		Mat img;
		undistort(imgs, img, m_cam_int, m_cam_dist);
		imgs = img;
	}

	////////////////////// Direct 3D based renderer //////////////////

	m_pd3dev->BeginScene();

	//////////////////// clear back buffer ///////////////////////////
	m_pd3dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0);

	/////////////////////// render main view //////////////////////////
	m_maincam.SetAsRenderTarget(m_pd3dev);

	m_maincam.blt_offsrf(m_pd3dev, imgs);

	renderObj();

	m_maincam.ResetRenderTarget(m_pd3dev);

	////////////////////// render 3D model ///////////////////////////

	switch(m_op){
	case MODEL:
	case OBJ:
		renderModel(timg);
	default:
		break;
	}

	//////////////////// render total view port /////////////////////
	m_maincam.show(m_pd3dev, (float)(0. + m_main_offset.x),
		(float) ((float) m_ViewPort.Height + m_main_offset.y), m_main_scale);

	switch(m_op){
	case MODEL:
		m_model_view.show(m_pd3dev, 0, (float) m_ViewPort.Height);
		break;
	case OBJ:
		m_model_view.show(m_pd3dev, 0, (float) m_ViewPort.Height, 0.25);
		break;
	default:
		break;
	}

	renderInfo();

	renderCursor();

	////////////////////// grab rendered back surface ////////////////
	if(m_grab_name)
		grab();

	////////////////////// presentation //////////////////////////////
	if(m_pd3dev->Present(NULL, NULL, 
		NULL, NULL) == D3DERR_DEVICELOST){
			cerr << "device lost" << endl;
			m_blost = true;
	}
}

// Drawing text based information to show filter state
void f_inspector::renderInfo()
{
	char information[1024];

	snprintf(information, 1023, 
		"OP=%s, MM=%d, Models=%d, CurModel=%d, Objs=%d, CurObj=%d, CurPoints=%d, CurPoint=%d",
		m_str_op[m_op], m_mm, m_models.size(),
		m_cur_model, m_obj.size(), 
		m_cur_obj, 
		(m_cur_obj < 0 ? m_cur_obj : m_obj[m_cur_obj].get_num_points()),
		m_cur_point);
	
	m_d3d_txt.render(m_pd3dev, information, 0, 0, 1.0, 0, EDTC_LT);

	snprintf(information, 1023, "MC(%d, %d)", m_mc.x, m_mc.y);

	m_d3d_txt.render(m_pd3dev, information, 0, 20, 1.0, 0, EDTC_LT);
}

// Decorating mouse cursor
void f_inspector::renderCursor()
{
	// vertial and horizontal lines are drawn as crossing at mouse cursor
	D3DXVECTOR2 v[2];
	m_pline->Begin();
	v[0] = D3DXVECTOR2((float) 0, (float) m_mc.y);
	v[1] = D3DXVECTOR2((float)m_ViewPort.Width - 1, (float) m_mc.y);
	m_pline->Draw(v, 2, D3DCOLOR_RGBA(0, 255, 0, 255));
	v[0] = D3DXVECTOR2((float) m_mc.x, (float) m_ViewPort.Height - 1);
	v[1] = D3DXVECTOR2((float) m_mc.x, (float) 0);
	m_pline->Draw(v, 2, D3DCOLOR_RGBA(0, 255, 0, 255));
	m_pline->End();
	m_pd3dev->EndScene();
}


void f_inspector::renderObj()
{
	// Drawing object (2d and 3d)
	for(int iobj = 0; iobj < m_obj.size(); iobj++){
		s_obj & obj = m_obj[iobj];
		if(m_op == OBJ){
			if(iobj == m_cur_obj){
				drawPoint2d(m_pd3dev, 
					NULL, m_pline,
					obj.get_pts(), obj.bvisible, iobj, 1);
			}else{
				drawPoint2d(m_pd3dev, 
					NULL, m_pline,
					obj.get_pts(), obj.bvisible, 0);
			}
		}else if(m_op == POINT){
			if(iobj == m_cur_obj){
				drawPoint2d(m_pd3dev, 
					NULL, m_pline,
					obj.get_pts(), obj.bvisible, 1, m_cur_point);
			}else{
				drawPoint2d(m_pd3dev, 
					NULL, m_pline,
					obj.get_pts(), obj.bvisible, 0);
			}			
		}

		m_obj[iobj].render(m_cam_int, m_cam_dist, m_rvec_cam, m_tvec_cam, 
			m_pd3dev, NULL, m_pline, 
			iobj, 0, m_cur_point);

		// render selected axis
		m_obj[iobj].render_axis(
			m_rvec_cam, m_tvec_cam, m_cam_int, m_cam_dist,
			m_pd3dev, m_pline, (int) m_axis);
	}
}

void f_inspector::renderModel(long long timg)
{
	m_model_view.SetAsRenderTarget(m_pd3dev);
	m_pd3dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0);
	// 3D model is rotated in the model view with the angle speed of 1 deg/sec
	// Camera parameter is set as 
	if(m_cur_model != -1){
		m_theta_z_mdl += (1./6.) * CV_PI / 180.;
		// calculating rotation vector
		m_rvec_mdl = Mat(3, 1, CV_64F);
		double * ptr = m_rvec_mdl.ptr<double>();
		ptr[0] = 0.;
		ptr[1] = m_theta_z_mdl;
		ptr[2] = 0.;

		// twice the maximum length of the model
		m_dist_mdl = 2 * m_models[m_cur_model].get_max_dist(); 
		
		// calculating translation vector
		m_tvec_mdl = Mat(3, 1, CV_64F);
		ptr = m_tvec_mdl.ptr<double>();
		ptr[0] = 0;
		ptr[1] = 0;
		ptr[2] = m_dist_mdl;

		// calculating camera matrix 
		// set the model at distance "dist" can be completely inside the view port
		// fov should be larger than 2*atan(0.5)
		// fov_w = atan(wpix * pitch / f)
		// fov_h = atan(hpix * pitch / f)
		// f = min((wpix * pitch)/tan(fov), (hpix * pitch)/tan(fov))
		// fpix = min(wpix, hpix) / tan(fov)
		float wpix = m_model_view.get_surface_width();
		float hpix = m_model_view.get_surface_height();
		float fpix = (float)(min(wpix, hpix) / tan(atan(0.5)));
		m_cam_int_mdl = Mat(3, 3, CV_64F);
		m_cam_int_mdl.at<double>(0, 0) = fpix;
		m_cam_int_mdl.at<double>(1, 1) = fpix;
		m_cam_int_mdl.at<double>(0, 2) = 0.5 * wpix;
		m_cam_int_mdl.at<double>(1, 2) = 0.5 * hpix;

		// calculating camera distortion (set at zero)
		m_cam_dist_mdl = Mat::zeros(8, 1, CV_64FC1);

		// calculating camera rotation
		m_rvec_cam_mdl = Mat::zeros(3, 1, CV_64FC1);

		// calculating camera translation (set as zero)
		m_tvec_cam_mdl = Mat::zeros(3, 1, CV_64FC1);
		
		vector<Point2f> pts;
		m_models[m_cur_model].proj(pts, m_cam_int_mdl, m_cam_dist_mdl, m_rvec_cam_mdl, m_tvec_cam_mdl, m_rvec_mdl, m_tvec_mdl);
		render_prjpts(m_models[m_cur_model], pts, m_pd3dev, NULL, m_pline, m_cur_model, 0, -1);	
	}
	m_model_view.ResetRenderTarget(m_pd3dev);
}

///////////////////////////////////////////////////////// message handler
// Planed features
// * Main window shows video image
// * Selected model is shown in subwindow (projected at the center, rotating around y-axis)
// * op = OBJ, enables adding new points.
//				Points are drawn. A selected point is highlighted. 
//				If a certaine model is assigned, the model instance is also rendered.
// * op = MODEL, enables model selection
// * op = POINT, enables point selection by left and right keys for current object.
// * op = OBJ3D, for selected object, enables rotating and translating in 3D space, and enable point matching between 2d and 3d. Drawings are the same as op=OBJ
// * op = POINT3D, enables point selection by left and right keys for current assigned 3D object.
// * op = CAMINT. manipulating camera interinsics displaying cimaging grid
// * op = CAMEXT manipulating camera extrinsic displaying world grid
// Current Implementation
// SHIFT + Drag : op=OBJ Scroll Video Image, op=OBJ3D x-y translation
// SHIFT + Wheel: op=OBJ Scaling, op=OBJ3D z translation
// Ctrl + Wheel: op=OBJ3D obj rotation for selected axis, op=CAMEXT camera rotation for selected axis
// Shift + Wheel: op=OBJ3D obj translation for selected axis, op=CAMEXT camera translation for selected axis
// L Click : Point add cur_obj
// Left Key: op=OBJ cur_obj--, op=POINT cur_obj_point--
// Right Key: op=OBJ cur_ob++, op=POINT cur_obj_point++
// F: Reset scale at original
// O: op=OBJ Add New Object 
// I: op=OBJ The current mdoel is assigned to the current object, then op<=OBJ3D
// C: op=OBJ The current point is assigned to the current 3d point.
// m: op <= MODEL
// o: op <= OBJ
// p: op <= POINT
// q: op <= OBJ3D
// r: op <= POINT3D
// e: op <= CAMEXT
// i: op <= CAMINT
// x: m_axis <= AX_X
// y: m_axis <= AX_Y
// z: m_axis <= AX_Z

void f_inspector::handle_lbuttondown(WPARAM wParam, LPARAM lParam)
{
	extractPointlParam(lParam, m_mc);
	switch(m_op){
	case OBJ:
	case POINT:
		if(GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT){ // scroll
			m_mm = MM_SCROLL;
			m_pt_sc_start = m_mc;
		}else{
			m_mm = MM_POINT;
		}
		break;
	case CAMERA:
		m_mm = MM_CAMINT;
		break;
	case MODEL:
		break;
	}
}

void f_inspector::handle_lbuttonup(WPARAM wParam, LPARAM lParam)
{
	extractPointlParam(lParam, m_mc);
	switch(m_mm){
	case MM_SCROLL:
		scroll_screen();
		break;
	case MM_CAMINT:
		shift_cam_center();
		break;
	case MM_POINT:
		assign_point2d();
		break;
	}
	m_mm = MM_NORMAL;
};

void f_inspector::assign_point2d()
{
	if(m_obj.size() == 0)
		return;

	if(m_cur_point < 0)
		return;

	Point2f pt;
	double iscale = 1.0 / m_main_scale;
	pt.x = (float)((m_mc.x - m_main_offset.x) * iscale);
	pt.y = (float)(m_mc.y - (int) m_ViewPort.Height - m_main_offset.y); 
	pt.y *= (float) iscale;
	pt.y += (float) m_ViewPort.Height;

	m_obj[m_cur_obj].pt2d[m_cur_point] = pt;
	m_obj[m_cur_obj].bvisible[m_cur_point] = true;
}

void f_inspector::handle_mousemove(WPARAM wParam, LPARAM lParam)
{
	extractPointlParam(lParam, m_mc);
	switch(m_mm){
	case MM_SCROLL:
		scroll_screen();
		break;
	case MM_CAMINT:
		shift_cam_center();
		break;
	default:
		break;
	}
}

void f_inspector::scroll_screen()
{
	m_main_offset.x += (float)(m_mc.x - m_pt_sc_start.x);
	m_main_offset.y += (float)(m_mc.y - m_pt_sc_start.y);
	m_pt_sc_start = m_mc;
}

void f_inspector::shift_cam_center()
{
	m_cam_int.at<double>(0, 2) += (float)(m_mc.x - m_pt_sc_start.x);
	m_cam_int.at<double>(1, 2)  += (float)(m_mc.y - m_pt_sc_start.y);
}

void f_inspector::handle_mousewheel(WPARAM wParam, LPARAM lParam)
{
	// Notice: Screen coordinate in WM_MOUSEWHEEL is different from other 
	// mouse related message. We need to subtract origin of the client screen
	// from the point sent by the message. 
	extractPointlParam(lParam, m_mc);
	m_mc.x -= m_client_org.x;
	m_mc.y -= m_client_org.y;

	short delta = GET_WHEEL_DELTA_WPARAM(wParam);
	if(GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT){
		switch(m_op){
		case OBJ:
		case POINT:
			zoom_screen(delta);
			break;
		}
	}else if(GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL){
		switch(m_op){
		case OBJ:
		case POINT:
			rotate_obj(delta);
			break;
		}
	}else{
		switch(m_op){
		case OBJ:
		case POINT:
			translate_obj(delta);
			break;
		case CAMERA:
			// change the focal length of the camera
			break;
		}
	}
}

void f_inspector::zoom_screen(short delta)
{
	short step = delta / WHEEL_DELTA;
	float scale = (float) pow(m_zoom_step, (double) step);
	m_main_scale *=  scale;
	float x, y;
	x = (float)(m_main_offset.x - m_mc.x) * scale;
	y = (float)((int) m_ViewPort.Height + m_main_offset.y - m_mc.y) * scale;
	m_main_offset.x =  (float)(x + (double) m_mc.x);
	m_main_offset.y =  (float)(y + (double) m_mc.y - (double) m_ViewPort.Height);
}

void f_inspector::translate_obj(short delta)
{
	if(m_cur_obj < 0)
		return ;

	double step = (double)(delta / WHEEL_DELTA) * m_trn_step;
	Mat tvec = Mat::zeros(3, 1, CV_64FC1);
	switch(m_axis){
	case AX_X:
		tvec.at<double>(0, 0) = step;
		break;
	case AX_Y:
		tvec.at<double>(1, 0) = step;
		break;
	case AX_Z:
		tvec.at<double>(2, 0) = step;
		break;
	}

	m_obj[m_cur_obj].tvec += tvec;
}

void f_inspector::rotate_obj(short delta)
{
	if(m_cur_obj < 0)
		return;

	// m_rot_step degree per wheel step
	double step = (double) (delta / WHEEL_DELTA) * (CV_PI / 180.) * m_rot_step;
	Mat rvec = Mat::zeros(3, 1, CV_64FC1);
	switch(m_axis){
	case AX_X:
		rvec.at<double>(0, 0) = step;
		break;
	case AX_Y:
		rvec.at<double>(1, 0) = step;
		break;
	case AX_Z:
		rvec.at<double>(2, 0) = step;
		break;
	}
	Mat R1, R2;
	Rodrigues(m_obj[m_cur_obj].rvec, R1);
	Rodrigues(rvec, R2);
	Mat R = R2 * R1;
	Rodrigues(R, m_obj[m_cur_obj].rvec);
}

void f_inspector::translate_cam(short delta)
{
	double step = (double)(delta / WHEEL_DELTA) * m_trn_step;
	Mat tvec = Mat::zeros(3, 1, CV_64FC1);
	switch(m_axis){
	case AX_X:
		tvec.at<double>(0, 0) = step;
		break;
	case AX_Y:
		tvec.at<double>(1, 0) = step;
		break;
	case AX_Z:
		tvec.at<double>(2, 0) = step;
		break;
	}

	m_tvec_cam += tvec;
}

void f_inspector::rotate_cam(short delta)
{
	// m_rot_step degree per wheel step
	double step = (double) (delta / WHEEL_DELTA) * (CV_PI / 180.) * m_rot_step;
	Mat rvec = Mat::zeros(3, 1, CV_64FC1);
	switch(m_axis){
	case AX_X:
		rvec.at<double>(0, 0) = step;
		break;
	case AX_Y:
		rvec.at<double>(1, 0) = step;
		break;
	case AX_Z:
		rvec.at<double>(2, 0) = step;
		break;
	}
	Mat R1, R2;
	Rodrigues(m_rvec_cam, R1);
	Rodrigues(rvec, R2);
	Mat R = R2 * R1;
	Rodrigues(R, m_rvec_cam);
}

void f_inspector::zoom_cam(short delta)
{
	short step = delta / WHEEL_DELTA;
	float scale = (float) pow(m_zoom_step, (double) step);

	m_cam_int.at<double>(0, 0) *= scale;
	m_cam_int.at<double>(1, 1) *= scale;
}

void f_inspector::handle_keydown(WPARAM wParam, LPARAM lParam)
{
	switch(wParam){
	case VK_DELETE:
		switch(m_op){
		case MODEL:
			// delete current Model
			if(m_cur_model >= 0){
				vector<s_model>::iterator itr = m_models.begin() + m_cur_model;
				m_models.erase(itr);
			}
			break;
		case OBJ:
			// delete current object
			if(m_cur_obj >= 0){
				vector<s_obj>::iterator itr = m_obj.begin() + m_cur_obj;
				m_obj.erase(itr);
			}
			break;
		case POINT:
			//reset current point
			if(m_cur_obj >= 0 && m_cur_point >= 0){
				s_obj & obj = m_obj[m_cur_obj];
				if(m_cur_point < obj.get_num_points())
					obj.bvisible[m_cur_point] = false;
			}
			break;
		case CAMERA:
			// delete current camera parameter
			break;
		}
		break;
	case VK_LEFT:
		handle_vk_left();
		break;
	case VK_RIGHT:
		handle_vk_right();
		break;
	default:
		break;
	}
}

void f_inspector::handle_vk_left()
{
	switch(m_op){
	case OBJ:
		{
			m_cur_obj = m_cur_obj - 1;
			if(m_cur_obj < 0){
				m_cur_obj += (int) m_obj.size();
			}
			// the current object point index is initialized
			m_cur_point = m_obj[m_cur_obj].get_num_points() - 1;
		}
		break;
	case POINT: // decrement current object point. 3d-object point is also. 
		m_cur_point = m_cur_point  - 1;
		if(m_cur_point < 0){
			m_cur_point += (int) m_obj[m_cur_obj].get_num_points();
		}

		break;
	case MODEL: // decrement the current model index
		m_cur_model = m_cur_model - 1;
		if(m_cur_model < 0){
			m_cur_model += (int) m_models.size();
		}
		break;
	case CAMERA:
		break;
	}
}

void f_inspector::handle_vk_right()
{
	switch(m_op){
	case OBJ:
		{
			m_cur_obj = m_cur_obj + 1;
			m_cur_obj %= (int) m_obj.size();
			m_cur_point = m_obj[m_cur_obj].get_num_points() - 1;
		}
		break;
	case POINT: // increment the current object point index. The 3d-object point index as well.
		m_cur_point = m_cur_point + 1;
		m_cur_point %= (int) m_obj[m_cur_obj].get_num_points();
		break;
	case MODEL: // increment the current model index.
		m_cur_model = m_cur_model + 1;
		m_cur_model %= (int) m_models.size();
		break;
	case CAMERA:
		break;
	}
}

void f_inspector::handle_char(WPARAM wParam, LPARAM lParam)
{
	switch(wParam){
	case 'R': /* F key*/
		m_main_offset = Point2f(0., 0.);
		m_main_scale = 1.0;
		break;
	case 'L':
		switch(m_op){
		case MODEL:
			load_model();
			break;
		case OBJ:
		case POINT:
			load_obj();
			break;
		case CAMERA:
			//load camera intrinsics
			break;
		}
		break;
	case 'S':
		switch(m_op){
		case OBJ:
		case POINT:
			save_obj();
			break;
		case CAMERA:
			//load camera intrinsics
			break;
		}
		break;
	case 'O': /* O key */ 
		m_op = OBJ;
		break;
	case 'P':
		m_op = POINT;
		break;
	case 'I':
		if(m_op != MODEL){
			if(m_cur_model < 0){
				break;
			}

			double width =(double) m_maincam.get_surface_width();
			double height = (double) m_maincam.get_surface_height();
			m_obj.push_back(s_obj(&m_models[m_cur_model], m_cam_int, m_cam_dist, width, height));
			m_cur_obj = (int) m_obj.size() - 1;
			m_op = OBJ;
			break;
		}
		break;
	case 'C':
		m_op = CAMERA;
		break;
	case 'x':
		m_axis = AX_X;
		break;
	case 'y':
		m_axis = AX_Y;
		break;
	case 'z':
		m_axis = AX_Z;
		break;
	case 'f':
		// fix parameter
		switch(m_op){
		case OBJ: 
		case POINT:
			// fix selected object's attitude
			break;
		case CAMERA:
			// fix selected camera parameter
		default:
			break;
		}

	default:
		break;
	}
}
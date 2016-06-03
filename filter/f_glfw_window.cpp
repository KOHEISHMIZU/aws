// Copyright(c) 2015 Yohei Matsumoto, Tokyo University of Marine
// Science and Technology, All right reserved. 

// f_glfw_window.cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_glfw_window.cpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_glfw_window.  If not, see <http://www.gnu.org/licenses/>. 
#include "stdafx.h"
#ifdef GLFW_WINDOW
#include <cstring>
#include <cmath>

#include <iostream>
#include <fstream>
#include <map>
using namespace std;

#include "../util/aws_stdlib.h"
#include "../util/aws_thread.h"
#include "../util/c_clock.h"

#include <opencv2/opencv.hpp>
using namespace cv;

#include "../util/aws_vobj.h"
#include "../util/aws_vlib.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GL/glut.h>
#include <GL/glu.h>

#include "f_glfw_window.h"

//////////////////////////////////////////////////////////////////////////////////////////// helper functions
void drawCvPoints(const Size & vp, vector<Point2f> & pts,
				  const float r, const float g, const float b, const float alpha, 
				  const float l /*point size*/)
{
	Point2f pt;
	double fac_x = 2.0 / (double) vp.width, fac_y = 2.0 / (double) vp.height;

	// drawing Points
	glPointSize(l);
	glBegin(GL_POINTS);
	{
		glColor4f(r, g, b, alpha);
		for(int ipt = 0; ipt < pts.size(); ipt++){
			cnvCvPoint2GlPoint(fac_x, fac_y, pts[ipt], pt);
			glVertex2f(pt.x, pt.y);
		}
	}
	glEnd();
}

void drawCvPoints(const float fac_x, const float fac_y, const float xorg, const float yorg, const float w, const float h, 
				  vector<Point2f> & pts, 
				  const float r, const float g, const float b, const float alpha, 
				  const float l /*point size*/)
{

	Point2f pt;
	// drawing Points
	glPointSize(l);
	glBegin(GL_POINTS);
	{
		glColor4f(r, g, b, alpha);
		for(int ipt = 0; ipt < pts.size(); ipt++){
			cnvCvPoint2GlPoint(fac_x, fac_y, xorg, yorg, w, h, pts[ipt], pt);
			glVertex2f(pt.x, pt.y);
		}
	}
	glEnd();

}

void drawCvChessboard(const Size & vp, vector<Point2f> & pts, 
					  const float r, const float g, const float b, const float alpha, 
					 const float l /* point size */, const float w /* line width */)
{
	glColor4f(r, g, b, alpha);
	glLineWidth(w);

	double fac_x = 2.0 / (double) vp.width, fac_y = 2.0 / (double) vp.height;

	glBegin(GL_LINES);
	{
		Point2f pt;
		cnvCvPoint2GlPoint(fac_x, fac_y, pts[0], pt);
		glVertex2f(pt.x, pt.y);
		for(int ipt = 1; ipt < pts.size(); ipt++){
			cnvCvPoint2GlPoint(fac_x, fac_y, pts[ipt], pt);
			glVertex2f(pt.x, pt.y);
		}
	}
	glEnd();
}

void drawCvChessboard(const float fac_x, const float fac_y, const float xorg, const float yorg, 
					  const float w, const float h, vector<Point2f> & pts, 
					  const float r, const float g, const float b, const float alpha, 
					 const float p /* point size */, const float l /* line width */)
{
	drawCvPoints(fac_x, fac_y, xorg, yorg, w, h, pts, r, g, b, alpha, p);
	glLineWidth(l);

	glBegin(GL_LINES);
	{
		Point2f pt;
		cnvCvPoint2GlPoint(fac_x, fac_y, xorg, yorg, w, h, pts[0], pt);
		glVertex2f(pt.x, pt.y);
		for(int ipt = 1; ipt < pts.size(); ipt++){
			cnvCvPoint2GlPoint(fac_x, fac_y, xorg, yorg, w, h, pts[ipt], pt);
			glVertex2f(pt.x, pt.y);
		}
	}
	glEnd();

}


void drawCvPointDensity(Mat hist, const int hist_max, const Size grid,  
				 const float r, const float g, const float b, const float alpha, 
				 const float w /* line width of the grid */)
{
	float fac = (float)(1.0 / (float) hist_max);
	float wgrid = (float)((float)2. / (float) grid.width);
	float hgrid = (float)((float)2. / (float) grid.height);

	glLineWidth(w);
	glBegin(GL_QUADS);
	{
		for(int y = 0; y < grid.height; y++){
			for(int x = 0; x < grid.width; x++){
				glColor4f(r * fac, g * fac, b * fac, alpha);
				float u, v;
				u = (float)(x * wgrid - 1.);
				v = -(float)(y * hgrid - 1.);
				glVertex2f(u, v);
				u = (float)((x + 1) * wgrid - 1.);
				glVertex2f(u, v);
				v = -(float)((y + 1) * hgrid - 1.);
				glVertex2f(u, v);
				u = (float)(x * wgrid - 1.);
				glVertex2f(u, v);
			}
		}
	}
	glEnd();
}

void drawGlText(float x, float y, const char * str, 
		float r, float g, float b , float alpha,
		void* font)
{
  glColor4f(r, g, b, alpha);
  int l = (int) strlen(str);
  glRasterPos2f(x, y);
  for(int i = 0; i < l; i++){
    glutBitmapCharacter(font, str[i]);
  }
}

void drawGlSquare2Df(float x1, float y1, float x2, float y2, 
		     float r, float g, float b, float alpha, float size)
{
  glColor4f(r, g, b, alpha);
  glLineWidth(size);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x1, y1);
  glVertex2f(x2, y1);
  glVertex2f(x2, y2);
  glVertex2f(x1, y2);
  glEnd();
}

void drawGlSquare2Df(float x1, float y1, float x2, float y2, 
		     float r, float g, float b, float alpha)
{
  glColor4f(r, g, b, alpha);
  glBegin(GL_QUADS);
  glVertex2f(x1, y1);
  glVertex2f(x2, y1);
  glVertex2f(x2, y2);
  glVertex2f(x1, y2); 
  glEnd();
}

void drawGlTriangle2Df(float x1, float y1, float x2, float y2, 
					float x3,float y3, float r, float g, float b, float alpha, float size)
{
  glColor4f(r, g, b, alpha);
  glLineWidth(size);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glVertex2f(x3, y3);
  glEnd();
}

void drawGlTriangle2Df(float x1, float y1, float x2, float y2, 
					float x3,float y3, float r, float g, float b, float alpha)
{
  glColor4f(r, g, b, alpha);
  glBegin(GL_TRIANGLES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glVertex2f(x3, y3);
  glEnd();
}

void drawGlPolygon2Df(Point2f * pts, int num_pts, 
					  float r, float g, float b, float alpha, float size)
{
  glColor4f(r, g, b, alpha);
  glLineWidth(size);
  glBegin(GL_LINE_LOOP);
  for(int i = 0; i<  num_pts; i++)
	  glVertex2f(pts[i].x, pts[i].y);
  glEnd();
}

void drawGlPolygon2Df(Point2f * pts, int num_pts, 
					  float r, float g, float b, float alpha)
{
  glColor4f(r, g, b, alpha);
  glBegin(GL_POLYGON);
  for(int i = 0; i <  num_pts; i++)
	  glVertex2f(pts[i].x, pts[i].y);
  glEnd();
}

void drawGlPolygon2Df(Point2f * pts, int num_pts, 					  
					  Point2f & offset,
					  float r, float g, float b, float alpha, float size)
{
  glColor4f(r, g, b, alpha);
  glLineWidth(size);
  glBegin(GL_LINE_LOOP);
  for(int i = 0; i<  num_pts; i++)
	  glVertex2f((float)(pts[i].x + offset.x), (float)(pts[i].y + offset.y));
  glEnd();
}

void drawGlPolygon2Df(Point2f * pts, int num_pts, 					  
					  Point2f & offset,
					  float r, float g, float b, float alpha)
{
  glColor4f(r, g, b, alpha);
  glBegin(GL_POLYGON);
  for(int i = 0; i <  num_pts; i++)
	  glVertex2f((float)(pts[i].x + offset.x), (float)(pts[i].y + offset.y));
  glEnd();
}


void drawGlLine2Df(float x1, float y1, float x2, float y2,
		   float r, float g, float b, float alpha, float size)
{
  glColor4f(r, g, b, alpha);
  glLineWidth(size);
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}


//////////////////////////////////////////////////////////////////////////////////////////// f_glfw_window
MapGLFWin f_glfw_window::m_map_glfwin;

f_glfw_window::f_glfw_window(const char * name):f_base(name), m_sz_win(640, 480)
{
	m_pwin = NULL;
	register_fpar("width", &m_sz_win.width, "Width of the window.");
	register_fpar("height", &m_sz_win.height, "Height of the window.");
}

f_glfw_window::~f_glfw_window()
{
}

bool f_glfw_window::init_run()
{
  if(!glfwInit()){
    cerr << "Failed to initialize GLFW." << endl;
    return false;
  }

  m_pwin = glfwCreateWindow(m_sz_win.width, m_sz_win.height, m_name, NULL, NULL);
  
  m_map_glfwin.insert(pair<GLFWwindow*, f_glfw_window*>(m_pwin, this));

	if (!pwin())
	{
	  cerr << "Failed to create GLFW window." << endl;
	  glfwTerminate();
	  return false;
	}

	glfwMakeContextCurrent(pwin());

	glfwSetKeyCallback(pwin(), key_callback);
	glfwSetCursorPosCallback(pwin(), cursor_position_callback);
	glfwSetMouseButtonCallback(pwin(), mouse_button_callback);
	glfwSetScrollCallback(pwin(), scroll_callback);
	glfwSetErrorCallback(err_cb);

	glfwSetWindowTitle(pwin(), m_name);

  GLenum err;
  if((err = glewInit()) != GLEW_OK){
    cerr << "Failed to initialize GLEW." << endl;
    cerr << "\tMessage: " << glewGetString(err) << endl;
    return false;
  }

  int argc = 0;
  glutInit(&argc, NULL);

	return true;	
}

void f_glfw_window::destroy_run()
{ 
  if(!m_pwin)
    return;
  glfwTerminate();
  MapGLFWin::iterator itr = m_map_glfwin.find(m_pwin);
  m_map_glfwin.erase(itr);
  m_pwin = NULL;
}

bool f_glfw_window::proc()
{
	if(glfwWindowShouldClose(pwin()))
		return false;

	//glfwMakeContextCurrent(pwin());
	
	// rendering codes >>>>>

	// <<<<< rendering codes

	glfwSwapBuffers(pwin());

	glfwPollEvents();

	return true;
}


//////////////////////////////////////////////////////// f_glfw_imview
bool f_glfw_imview::proc()
{
  glfwMakeContextCurrent(pwin());

	if(glfwWindowShouldClose(pwin()))
		return false;

	long long timg;
	Mat img = m_pin->get_img(timg);
	if(img.empty())
		return true;
	if(m_timg == timg)
		return true;

	m_timg = timg;

	if(m_sz_win.width != img.cols || m_sz_win.height != img.rows){
	  Mat tmp;
	  resize(img, tmp, m_sz_win);
	  img = tmp;
	}

	glRasterPos2i(-1, -1);
	
	if(img.type() == CV_8U){
	  glDrawPixels(img.cols, img.rows, GL_LUMINANCE, GL_UNSIGNED_BYTE, img.data);
	}
	else{
	  cnvCVBGR8toGLRGB8(img);
	  glDrawPixels(img.cols, img.rows, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	}

	float hfont = (float)(24. / (float) m_sz_win.height);
	float wfont = (float)(24. / (float) m_sz_win.height);
	float x = wfont - 1;
	float y = 1 - 2 * hfont;

	// show time
	drawGlText(x, y, m_time_str, 0, 1, 0, 1, GLUT_BITMAP_TIMES_ROMAN_24);

	glfwSwapBuffers(pwin());
	glfwPollEvents();

	return true;
}


bool f_glfw_imview::init_run()
{
  if(m_chin.size() == 0){
    cerr << m_name << " requires at least an input channel." << endl;
    return false;
  }		
	m_pin = dynamic_cast<ch_image*>(m_chin[0]);
	if(m_pin == NULL){
	  cerr << m_name << "'s first input channel should be image channel." << endl;
	  return false;
	}

	if(!f_glfw_window::init_run())
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////f_glfw_stereo_view members
f_glfw_stereo_view::f_glfw_stereo_view(const char * name): f_glfw_window(name), m_pin1(NULL), m_pin2(NULL),
	m_timg1(-1), m_timg2(-1),
	m_bchsbd(false), m_num_chsbdl(0), m_num_chsbdr(0), m_num_chsbd_com(0), m_bflipx(false), m_bflipy(false),
	m_bcpl(false), m_bcpr(false),m_bsvcp(false), m_bldcp(false), m_budl(false), m_budr(false), m_bdisp(false),
	m_bcbl(false), m_bcbr(false), m_bcbst(false), 
	m_brct(false), m_bsv_chsbd(false), m_bld_chsbd(false), m_bdet_chsbd(false), m_bsvstp(false), m_bldstp(false),
	m_bfisheye(false), m_bfix_int(false), m_bfix_k1(false), m_bfix_k2(false), m_bfix_k3(false),
	m_bfix_k4(false), m_bfix_k5(false), m_bfix_k6(false), m_bguess_int(false), m_bfix_ar(false),
	m_bfix_pp(false), m_bzr_tng(false), m_brat_mdl(false), m_bred_chsbd(false), m_num_calib_chsbd(50)
{
	register_fpar("caml", (ch_base**)&m_pin1, typeid(ch_image_ref).name(), "Left camera channel");
	register_fpar("camr", (ch_base**)&m_pin2, typeid(ch_image_ref).name(), "Right camera channel");
	
	m_fcpl[0] = m_fcpr[0] = m_fstp[0] = '\0';
	register_fpar("fcpl", m_fcpl, 1024, "Camera parameter file of left camera.");
	register_fpar("fcpr", m_fcpr, 1024, "Camera parameter file of right camera.");
	register_fpar("fstp", m_fstp, 1024, "Stereo parameter file.");
	register_fpar("udl", &m_budl, "Undistort left camera");
	register_fpar("udr", &m_budr, "Undistort right camera");
	register_fpar("cbl", &m_bcbl, "Calibrate left camera");
	register_fpar("cbr", &m_bcbr, "Calibrate right camera");
	register_fpar("cbst", &m_bcbst, "Calibrate stereo images.");
	register_fpar("sv_chsbd", &m_bsv_chsbd, "Save chessboard.");
	register_fpar("ld_chsbd", &m_bld_chsbd, "Load chessboard.");
	register_fpar("det_chsbd", &m_bdet_chsbd, "Detect chessboard.");
	register_fpar("red_chsbd", &m_bred_chsbd, "Reduce chessboard to calib_chsbds");
	register_fpar("svcp", &m_bsvcp, "Save camera parameter.");
	register_fpar("ldcp", &m_bldcp, "Load camera parameter.");
	register_fpar("svstp", &m_bsvstp, "Save stereo parameter.");
	register_fpar("ldstp", &m_bldstp, "Load stereo parameter.");

	register_fpar("flipx", &m_bflipx, "Flip image in x");
	register_fpar("flipy", &m_bflipy, "Flip image in y");

	register_fpar("fchsbd", m_chsbd.fname, 1024, "Chessboard model file");
	m_fchsbdl[0] = m_fchsbdr[0] = m_fchsbdc[0] = '\0';
	
	register_fpar("fchsbdl", m_fchsbdl, 1024, "Detected chessboard file for left camera.");
	register_fpar("fchsbdr", m_fchsbdr, 1024, "Detected chessboard file for righ camera.");
	register_fpar("fchsbdc", m_fchsbdc, 1024, "Detected common chessboard file for left and right camera.");

	register_fpar("calib_chsbds", &m_num_calib_chsbd, "Number of chessboards used for calibration.");
	register_fpar("guess_int", &m_bguess_int, "Use intrinsic guess.");
	register_fpar("fisheye", &m_bfisheye, "Fisheye model is used.");
	register_fpar("fix_int", &m_bfix_int, "Fix intrinsic parameters");
	register_fpar("fix_pp", &m_bfix_pp, "Fix camera center as specified (cx, cy)");
	register_fpar("fix_aspect_ratio", &m_bfix_ar, "Fix aspect ratio as specified fx/fy. Only fy is optimized.");
	register_fpar("zero_tangent_dist", &m_bzr_tng, "Zeroify tangential distortion (px, py)");
	register_fpar("fix_k1", &m_bfix_k1, "Fix k1 as specified.");
	register_fpar("fix_k2", &m_bfix_k2, "Fix k2 as specified.");
	register_fpar("fix_k3", &m_bfix_k3, "Fix k3 as specified.");
	register_fpar("fix_k4", &m_bfix_k4, "Fix k4 as specified.");
	register_fpar("fix_k5", &m_bfix_k5, "Fix k5 as specified.");
	register_fpar("fix_k6", &m_bfix_k6, "Fix k6 as specified.");
	register_fpar("rat_mdl", &m_brat_mdl, "Enable rational model (k4, k5, k6)");
	register_fpar("fxl", (m_camparl.getCvPrj() + 0), "Focal length in x");
	register_fpar("fyl", (m_camparl.getCvPrj() + 1), "Focal length in y");
	register_fpar("cxl", (m_camparl.getCvPrj() + 2), "Principal point in x");
	register_fpar("cyl", (m_camparl.getCvPrj() + 3), "Principal point in y");
	register_fpar("k1l", (m_camparl.getCvDist() + 0), "k1 for left camera");
	register_fpar("k2l", (m_camparl.getCvDist() + 1), "k2 for left camera");
	register_fpar("p1l", (m_camparl.getCvDist() + 2), "p1 for left camera");
	register_fpar("p2l", (m_camparl.getCvDist() + 3), "p2 for left camera");
	register_fpar("k3l", (m_camparl.getCvDist() + 4), "k3 for left camera");
	register_fpar("k4l", (m_camparl.getCvDist() + 5), "k4 for left camera");
	register_fpar("k5l", (m_camparl.getCvDist() + 6), "k5 for left camera");
	register_fpar("k6l", (m_camparl.getCvDist() + 7), "k6 for left camera");
	register_fpar("k1fl", (m_camparl.getCvDistFishEye() + 0), "k1 for left camera with fisheye");
	register_fpar("k2fl", (m_camparl.getCvDistFishEye() + 1), "k2 for left camera with fisheye");
	register_fpar("k3fl", (m_camparl.getCvDistFishEye() + 2), "k3 for left camera with fisheye");
	register_fpar("k4fl", (m_camparl.getCvDistFishEye() + 3), "k4 for left camera with fisheye");

	register_fpar("fxr", (m_camparr.getCvPrj() + 0), "Focal length in x");
	register_fpar("fyr", (m_camparr.getCvPrj() + 1), "Focal length in y");
	register_fpar("cxr", (m_camparr.getCvPrj() + 2), "Principal point in x");
	register_fpar("cyr", (m_camparr.getCvPrj() + 3), "Principal point in y");
	register_fpar("k1r", (m_camparr.getCvDist() + 0), "k1 for right camera");
	register_fpar("k2r", (m_camparr.getCvDist() + 1), "k2 for right camera");
	register_fpar("p1r", (m_camparr.getCvDist() + 2), "p1 for right camera");
	register_fpar("p2r", (m_camparr.getCvDist() + 3), "p2 for right camera");
	register_fpar("k3r", (m_camparr.getCvDist() + 4), "k3 for right camera");
	register_fpar("k4r", (m_camparr.getCvDist() + 5), "k4 for right camera");
	register_fpar("k5r", (m_camparr.getCvDist() + 6), "k5 for right camera");
	register_fpar("k6r", (m_camparr.getCvDist() + 7), "k6 for right camera");
	register_fpar("k1fr", (m_camparr.getCvDistFishEye() + 0), "k1 for right camera with fisheye");
	register_fpar("k2fr", (m_camparr.getCvDistFishEye() + 1), "k2 for right camera with fisheye");
	register_fpar("k3fr", (m_camparr.getCvDistFishEye() + 2), "k3 for right camera with fisheye");
	register_fpar("k4fr", (m_camparr.getCvDistFishEye() + 3), "k4 for right camera with fisheye");

	m_Rl = Mat::eye(3, 3, CV_64FC1);
	m_Rr = Mat::eye(3, 3, CV_64FC1);

	m_pts_chsbdr.reserve(200);
	m_pts_chsbdl.reserve(200);
	m_ifrm_chsbdr.reserve(200);
	m_ifrm_chsbdl.reserve(200);
	m_pts_chsbdr_com.reserve(200);
	m_pts_chsbdl_com.reserve(200);
	m_ifrm_chsbd_com.reserve(200);
}

bool f_glfw_stereo_view::proc()
{
  glfwMakeContextCurrent(pwin());

  if(glfwWindowShouldClose(pwin()))
    return false;
  
  long long timg1, timg2, ifrm1, ifrm2;
  m_img1 = m_pin1->get_img(timg1, ifrm1);
  m_img2 = m_pin2->get_img(timg2, ifrm2);

  if(m_img1.type() != m_img2.type()){
    cerr << "Two image channels in " << m_name << " have different image type." << endl;
     return false;
  }

  if(m_img1.empty() || m_img2.empty()){
    cerr << "One of the image channels are empty." << endl;
    return true;
  }

  if(m_timg1 == timg1 && m_timg2 == timg2) // no frame change
	  m_bnew = false;
  else
	  m_bnew = true;

  if(ifrm1 != ifrm2){
	  m_bsync = false;
  }else{
	  m_bsync = true;
  }

  if(m_num_chsbdl && m_bcbl){ //calibrate left camera
	  calibrate(0);
	  m_bcbl = false;
  }

  if(m_num_chsbdr && m_bcbr){ // calibrate right camera
	  calibrate(1);
	  m_bcbr = false;
  }

  if(m_bcpl && m_bcpr && m_bcbst){ // stereo calibrate
	  Size sz(m_img1.cols, m_img1.rows);
	  vector<vector<Point3f>> pt3ds;
	  pt3ds.resize(m_num_chsbd_com);
	  for(int i = 0; i < m_num_chsbd_com; i++)
		  pt3ds[i] = m_chsbd.pts;
	  if(m_bfisheye){
		  Mat Kl, Dl, Kr, Dr;
		  Kl = m_camparl.getCvPrjMat().clone();
		  Dl = m_camparl.getCvDistFishEyeMat().clone();
		  Kr = m_camparr.getCvPrjMat().clone();
		  Dr = m_camparr.getCvDistFishEyeMat().clone();
		  fisheye::stereoCalibrate(pt3ds, m_pts_chsbdl_com, m_pts_chsbdr_com, 
			  Kl, Dl, Kr, Dr, sz, m_Rlr, m_Tlr);
		  fisheye::stereoRectify(Kl, Dl, Kr, Dr, sz, m_Rlr, m_Tlr, m_Rl, m_Rr, m_Pl, m_Pr, m_Q, 0);
	  }else{
		  Mat Kl, Dl, Kr, Dr;
		  Kl = m_camparl.getCvPrjMat().clone();
		  Dl = m_camparl.getCvDistMat().clone();
		  Kr = m_camparr.getCvPrjMat().clone();
		  Dr = m_camparr.getCvDistMat().clone();
		  stereoCalibrate(pt3ds, m_pts_chsbdl_com, m_pts_chsbdr_com, 
			  Kl, Dl, Kr, Dr, sz, m_Rlr, m_Tlr, m_E, m_F);
		  stereoRectify(Kl, Dl, Kr, Dr, sz, m_Rlr, m_Tlr, m_Rl, m_Rr, m_Pl, m_Pr, m_Q, 0);
	  }
	  init_undistort(m_camparl, sz, m_Rl, m_Pl, m_mapl1, m_mapl2, m_bcpl);
	  init_undistort(m_camparr, sz, m_Rr, m_Pr, m_mapr1, m_mapr2, m_bcpr);
	  cout << "Rl" << m_Rl << endl;
	  cout << "Pl" << m_Pl << endl;
	  cout << "Rr" << m_Rr << endl;
	  cout << "Pr" << m_Pr << endl;
	  m_bcbst = false;
	  m_brct = true;
  }

  m_timg1 = timg1;
  m_timg2 = timg2;

  Mat img1, img2;
  if(m_budl && m_bcpl){ 
	  remap(m_img1, img1, m_mapl1, m_mapl2, CV_INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));
  }else{
	  img1 = m_img1.clone();
  }

  if(m_budr && m_bcpr){
	  remap(m_img2, img2, m_mapr1, m_mapr2, CV_INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));
  }else{
	  img2 = m_img2.clone();
  }

  if(m_bldcp){
	  m_bcpl = m_camparl.read(m_fcpl);
	  Size sz(m_img1.cols, m_img1.rows);
	  init_undistort(m_camparl, sz, m_Rl, m_Pl, m_mapl1, m_mapl2, m_bcpl);
	  m_bcpr = m_camparr.read(m_fcpr);
	  sz.width = m_img2.cols;
	  sz.height = m_img2.rows;
	  init_undistort(m_camparr, sz, m_Rr, m_Pr, m_mapr1, m_mapr2, m_bcpr);
	  m_bldcp = false;
  }

  if(m_bsvcp){
	  m_bcpr = m_camparl.write(m_fcpl);
	  m_bcpr = m_camparr.write(m_fcpr);
	  m_bsvcp = false;
  }

  awsFlip(img1, m_bflipx, m_bflipy, false);
  awsFlip(img2, m_bflipx, m_bflipy, false);

  if(m_bchsbd && m_bdet_chsbd && m_bsync){
	  s_obj obj;
	  bool left = false, right = false;
	  // seraching chessboard detected in the frame
	  for(int i = 0; i < m_ifrm_chsbdl.size(); i++){
		  if(m_ifrm_chsbdl[i] == ifrm1){
			  left = true;
		  }
	  }
	  for(int i = 0; i < m_ifrm_chsbdr.size(); i++){
		  if(m_ifrm_chsbdr[i] == ifrm2){
			  right = true;
		  }
	  }
	  
	  if(!left || !right){
		  if(!left && m_chsbd.detect(img1, &obj)){
			  m_pts_chsbdl.push_back(obj.pt2d);		  
			  m_ifrm_chsbdl.push_back(ifrm1);
			  m_num_chsbdl++;
			  cout << "CamL frm[" << ifrm1 << "] chsbd[" << m_num_chsbdl << "] found." << endl;
			  left = true;
		  }
		  if(!right && m_chsbd.detect(img2, &obj)){
			  m_pts_chsbdr.push_back(obj.pt2d);
			  m_ifrm_chsbdr.push_back(ifrm2);
			  cout << "CamR frm[" << ifrm2 << "] chsbd[" << m_num_chsbdr << "] found." << endl;
			  m_num_chsbdr++;
			  right = true;
		  }

		  if(left && right){ // common chessboard		  
			  cout << "frm[" << ifrm1 << "] common chsbd[" << m_num_chsbd_com << "] found." << endl;
			  m_pts_chsbdr_com.push_back(m_pts_chsbdr[m_num_chsbdr - 1]);
			  m_pts_chsbdl_com.push_back(m_pts_chsbdl[m_num_chsbdl - 1]);
			  m_ifrm_chsbd_com.push_back(ifrm1);
			  m_num_chsbd_com++;
		  }
		  m_bdet_chsbd = false;
	  }
  }

  if(m_bred_chsbd){ // reduce chessboard less than m_num_calib_chsbd
	  reduce_chsbd(0);
	  reduce_chsbd(1);
	  reduce_chsbd(2);
  }

  Mat wimg1, wimg2;
  
  double rx1, rx2, ry1, ry2;
  int w = m_sz_win.width >> 1, h = m_sz_win.height >> 1;
  rx1 = (double)w / (double)m_img1.cols; 
  rx2 = (double)w / (double)m_img2.cols;
  ry1 = (double)h / (double)m_img1.rows;
  ry2 = (double)h / (double)m_img2.rows;
  double r1 = min(rx1, ry1), r2 = min(rx2, ry2);

  Size sz1((int)(r1 * img1.cols), (int)(r1 * img1.rows)), sz2((int)(r2 * img2.cols), (int)(r2 * img2.rows));
  resize(img1, wimg1, sz1);
  resize(img2, wimg2, sz2);
  
  glRasterPos2i(-1, 0);
  
  awsFlip(wimg1, false, true, false);
  switch(wimg1.type()){
  case CV_8U:
	  glDrawPixels(wimg1.cols, wimg1.rows, GL_LUMINANCE, GL_UNSIGNED_BYTE, wimg1.data);
	  break;
  case CV_16U:
	  glDrawPixels(wimg1.cols, wimg1.rows, GL_LUMINANCE, GL_UNSIGNED_SHORT, wimg1.data);
	  break;
  case CV_8UC3:
	  glDrawPixels(wimg1.cols, wimg1.rows, GL_BGR, GL_UNSIGNED_BYTE, wimg1.data);
	  break;
  }

  glRasterPos2i(0, 0);
  awsFlip(wimg2, false, true, false);
  switch(wimg2.type()){
  case CV_8U:
	  glDrawPixels(wimg2.cols, wimg2.rows, GL_LUMINANCE, GL_UNSIGNED_BYTE, wimg2.data);
	  break;
  case CV_16U:
	  glDrawPixels(wimg2.cols, wimg2.rows, GL_LUMINANCE, GL_UNSIGNED_SHORT, wimg2.data);
	  break;
  case CV_8UC3:
	  glDrawPixels(wimg2.cols, wimg2.rows, GL_BGR, GL_UNSIGNED_BYTE, wimg2.data);
	  break;
  }

  float xscale1, yscale1, xscale2, yscale2;
  xscale1 = (float)(r1 / (float) w);
  yscale1 = (float)(r1 / (float) h);
  xscale2 = (float)(r2 / (float) w);
  yscale2 = (float)(r2 / (float) h);
  if(m_num_chsbdl){
	  draw_chsbd(1., 0, 0, xscale1, yscale1, -1, 0, 
		  (float)((double) sz1.width / (double) w), (float)((double) sz1.height / (double) h), 
		  m_pts_chsbdl, m_num_chsbdl);
  } 

  if(m_num_chsbdr){
	  draw_chsbd(1., 0, 0, xscale2, yscale2, 0, 0, 
		  (float)((double) sz2.width / (double) w), (float)((double) sz2.height / (double) h), 
		  m_pts_chsbdr, m_num_chsbdr);
  }

  if(m_num_chsbd_com){
	  draw_chsbd(0, 0, 1, xscale1, yscale1, -1, 0, 
		  (float)((double) sz1.width / (double) w), (float)((double) sz1.height / (double) h), 
		  m_pts_chsbdl_com, m_num_chsbd_com);
	  draw_chsbd(0, 0, 1, xscale2, yscale2, 0, 0, 
		  (float)((double) sz2.width / (double) w), (float)((double) sz2.height / (double) h), 
		  m_pts_chsbdr_com, m_num_chsbd_com);
  }

  if(m_bsv_chsbd){
	  save_chsbd(0);
	  save_chsbd(1);
	  save_chsbd(2);
	  m_bsv_chsbd = false;
  }

  if(m_bld_chsbd){
	  load_chsbd(0);
	  load_chsbd(1);
	  load_chsbd(2);
	  m_bld_chsbd = false;
  }

  if(m_bsvstp && m_brct){
	  save_stereo_pars();
	  m_bsvstp = false;
  }

  if(m_bldstp){
	  load_stereo_pars();
	  m_bldstp = false;
  }

  if(m_bdisp && m_brct){ // show disparity map

  }

  glfwSwapBuffers(pwin());
  glfwPollEvents();
  
  return true;
}

bool f_glfw_stereo_view::init_run()
{
  if(!f_glfw_window::init_run())
    return false;
  
  if(m_bchsbd = m_chsbd.load())
  {
	  if(m_chsbd.type == s_model::EMT_CHSBD){
		  cout << "Chessboard is loaded." << endl;
		  cout << m_chsbd.par_chsbd.w << "x" <<  m_chsbd.par_chsbd.h << " " << m_chsbd.par_chsbd.p << "m pitch" << endl;
	  }
  }

  return true;
}

void f_glfw_stereo_view::save_chsbd(int icam)
{
	int num_chsbd = 0;
	int num_chsbd_per_frm = 0;
	const char * fname = NULL;
	vector<long long> *ifrm = NULL;
	vector<vector<Point2f>> *pts[2] = {NULL, NULL};

	switch(icam){
	case 0:
		num_chsbd = m_num_chsbdl;
		num_chsbd_per_frm = 1;
		fname = m_fchsbdl;
		ifrm = &m_ifrm_chsbdl;
		pts[0] = &m_pts_chsbdl;
		break;
	case 1:
		num_chsbd = m_num_chsbdr;
		num_chsbd_per_frm = 1;
		fname = m_fchsbdr;
		ifrm = &m_ifrm_chsbdr;
		pts[0] = &m_pts_chsbdr;
		break;
	case 2:
		num_chsbd = m_num_chsbd_com;
		num_chsbd_per_frm = 2;
		fname = m_fchsbdc;
		ifrm = &m_ifrm_chsbd_com;
		pts[0] = &m_pts_chsbdl_com;
		pts[1] = &m_pts_chsbdr_com;
		break;
	default:
		return;
	}

	FileStorage fs;
	fs.open(fname, FileStorage::WRITE);
	if(!fs.isOpened()){
		cerr << "Failed to open file " << fname << "." << endl;
		return;
	}

	fs << "NumChsbd" << num_chsbd;
	fs << "NumChsbdPerFrm" << num_chsbd_per_frm;
	fs << "ChsbdName" << m_chsbd.name;
	int chsbd_pts = (int) m_chsbd.pts.size();
	char item[1024];
	for(int i = 0; i < num_chsbd; i++){
		snprintf(item, 1024, "frm%d", i);
		fs << item << "{";
		long long val = (*ifrm)[i];
		fs << "FrmIdL" << *((int *) &val);
		fs << "FrmIdH" << *((int *) &val + 1);
		for(int j = 0; j < num_chsbd_per_frm; j++){
			snprintf(item, 1024, "chsbd%d", j);
			fs << item << "[";
			for(int k = 0; k < (*pts[j])[i].size(); k++){
				fs << (*pts[j])[i][k];
			}
			fs << "]";
		}
		fs << "}";
	}
}

void f_glfw_stereo_view::load_chsbd(int icam)
{
	int num_chsbd = 0;
	int num_chsbd_per_frm = 0;
	const char * fname = NULL;
	vector<long long> *ifrm = NULL;
	vector<vector<Point2f>> *pts[2] = {NULL, NULL};
	switch(icam){
	case 0:
		fname = m_fchsbdl;
		ifrm = &m_ifrm_chsbdl;
		pts[0] = &m_pts_chsbdl;
		break;
	case 1:
		ifrm = &m_ifrm_chsbdl;
		fname = m_fchsbdr;
		pts[0] = &m_pts_chsbdr;
		break;
	case 2:
		fname = m_fchsbdc;
		ifrm = &m_ifrm_chsbd_com;
		pts[0] = &m_pts_chsbdl_com;
		pts[1] = &m_pts_chsbdr_com;
		break;
	default:
		return;
	}

	string str;
	FileStorage fs(fname, FileStorage::READ);
	FileNode fn;

	fn = fs["NumChsbd"];
	if(fn.empty()){
		cerr << "Item \"NumChsbd\" cannot be found." << endl;
		return;
	}
	fn >> num_chsbd;

	fn = fs["NumChsbdPerFrm"];
	if(fn.empty()){
		cerr << "Item \"NumChsbdPerFrm\" cannot be found." << endl;
		return;
	}
	fn >> num_chsbd_per_frm;

	if(icam == 2 && num_chsbd_per_frm != 2){
		cerr << "File " << fname  << " is not ready for stereo calibration." << endl;
		return;
	}

	fn = fs["ChsbdName"];
	if(fn.empty()){
		cerr << "Item \"ChsbdName\" cannot be found." << endl;
		return;
	}
	fn >> str;

	if(str != m_chsbd.name){
		cerr << "Miss match in chessboard model." << endl;
		return;
	}

	(*ifrm).resize(num_chsbd);
	(*pts[0]).resize(num_chsbd);
	if(pts[1]){
		(*pts[1]).resize(num_chsbd);
	}

	int chsbd_pts = (int) m_chsbd.pts.size();
	char item[1024];
	for(int i = 0; i < num_chsbd; i++){
		snprintf(item, 1024, "frm%d", i);
		FileNode frm = fs[item];
		if(frm.empty()){
			cerr << item << " cannot be found." << endl;
			return;
		}

		int valL, valH;
		long long val;
		FileNode frm_id = frm["FrmIdL"];		
		if(frm_id.empty()){
			cerr << "\"FrmIdL\" is expected." << endl;
			return;
		}
		
		frm_id >> valL;

		frm_id = frm["FrmIdH"];
		if(frm_id.empty()){
			cerr << "\"FrmIdH\" is expected." << endl;
		}
		frm_id >> valH;
				
		*((int *) &val) = valL;
		*((int *) &val + 1) = valH;
		(*ifrm)[i] = val;

		for(int j = 0; j < num_chsbd_per_frm; j++){
			(*pts[j])[i].resize(chsbd_pts);
			snprintf(item, 1024, "chsbd%d", j);
			FileNode chsbd = frm[item];
			if(chsbd.empty()){
				cerr << item << " cannot be found." << endl;
				return;
			}

			FileNodeIterator itr = chsbd.begin();
			for(int k = 0;itr != chsbd.end(); itr++, k++){
				*itr >> (*pts[j])[i][k];
			}
		}
	}

	switch(icam){
	case 0:
		m_num_chsbdl = num_chsbd;
		break;
	case 1:
		m_num_chsbdr = num_chsbd;
		break;
	case 2:
		m_num_chsbd_com = num_chsbd;
		break;
	default:
		return;
	}

}

void f_glfw_stereo_view::save_stereo_pars()
{
	if(!m_fstp[0]){
		return;
	}
	FileStorage fs(m_fstp, FileStorage::WRITE);
	if(!fs.isOpened()){
		return;
	}
	fs << "Rlr" << m_Rlr;
	fs << "Tlr" << m_Tlr;
	fs << "Pl" << m_Pl;
	fs << "Pr" << m_Pr;
	fs << "Rl" << m_Rl;
	fs << "Rr" << m_Rr;
	fs << "E" << m_E;
	fs << "F" << m_F;
	fs << "Q" << m_Q;
	fs << "mapl1" << m_mapl1;
	fs << "mapl2" << m_mapl2;
	fs << "mapr1" << m_mapr1;
	fs << "mapr2" << m_mapr2;
}

void f_glfw_stereo_view::load_stereo_pars()
{
	if(!m_fstp[0]){
		return;
	}
	FileStorage fs(m_fstp, FileStorage::READ);
	if(!fs.isOpened()){
		return;
	}

	FileNode fn;

	fn = fs["Rlr"];
	if(fn.empty())
		return;
	fn >> m_Rlr;

	fn = fs["Tlr"];
	if(fn.empty())
		return;
	fn >> m_Tlr;

	fn = fs["Pl"];
	if(fn.empty())
		return;
	fn >> m_Pl;

	fn = fs["Pr"];
	if(fn.empty())
		return;
	fn >> m_Pr;

	fn = fs["Rl"];
	if(fn.empty())
		return;
	fn >> m_Rl;

	fn = fs["Rr"];
	if(fn.empty())
		return;
	fn >> m_Rr;

	fn = fs["E"];
	if(fn.empty())
		return;
	fn >> m_E;

	fn = fs["F"];
	if(fn.empty())
		return;
	fn >> m_F;

	fn = fs["Q"];
	if(fn.empty())
		return;
	fn >> m_Q;

	fn = fs["mapl1"];
	if(fn.empty())
		return;
	fn >> m_mapl1;

	fn = fs["mapl2"];
	if(fn.empty())
		return;
	fn >> m_mapl2;

	fn = fs["mapr1"];
	if(fn.empty())
		return;
	fn >> m_mapr1;

	fn = fs["mapr2"];
	if(fn.empty())
		return;
	fn >> m_mapr2;

	m_brct = true;
}


void f_glfw_stereo_view::reduce_chsbd(int icam)
{
	int num_chsbd = 0;
	int num_chsbd_per_frm = 0;
	const char * fname = NULL;
	vector<long long> *ifrm = NULL;
	vector<vector<Point2f>> *pts[2] = {NULL, NULL};
	int w, h;
	switch(icam){
	case 0:
		num_chsbd = m_num_chsbdl;
		num_chsbd_per_frm = 1;
		fname = m_fchsbdl;
		ifrm = &m_ifrm_chsbdl;
		pts[0] = &m_pts_chsbdl;
		w = m_img1.cols;
		h = m_img1.rows;
		break;
	case 1:
		num_chsbd = m_num_chsbdr;
		num_chsbd_per_frm = 1;
		fname = m_fchsbdr;
		ifrm = &m_ifrm_chsbdr;
		pts[0] = &m_pts_chsbdr;
		w = m_img2.cols;
		h = m_img2.rows;
		break;
	case 2:
		num_chsbd = m_num_chsbd_com;
		num_chsbd_per_frm = 2;
		fname = m_fchsbdc;
		ifrm = &m_ifrm_chsbd_com;
		pts[0] = &m_pts_chsbdl_com;
		pts[1] = &m_pts_chsbdr_com;
		w = m_img1.cols;
		h = m_img1.rows;
		break;
	default:
		return;
	}

	
	// dividing image into 8x8 grid, and count the chessboard points found in each grid
	float iws = (float)(1.0 / (double)(w >> 3)), ihs = (float)(1.0  / (double)(h >> 3));
	vector<vector<int>> hist[2];
	vector<s_chsbd_score> cs;
	hist[0].resize(8);
	cs.resize(pts[0]->size());
	for(int i = 0; i < num_chsbd; i++){
		cs[i].idx = i;
		cs[i].score = 0;
	}

	if(num_chsbd_per_frm == 2){
		hist[1].resize(8);
	}

	for(int i = 0; i < 8; i++){
		hist[0][i].resize(8, 0);
		if(num_chsbd_per_frm == 2){
			hist[1][i].resize(8, 0);
		}
	}

	// voting phase
	for(int j = 0; j < num_chsbd_per_frm; j++){
		vector<vector<int>> & _hist = hist[j];
		vector<vector<Point2f>> & _pts = *pts[j];
		for(int k = 0; k < _pts.size(); k++){
			vector<Point2f> & _chsbd = _pts[k];
			for(int l = 0; l < _chsbd.size(); l++){
				Point2f & pt = _chsbd[l];
				int x = (int) (pt.x * iws);
				int y = (int) (pt.y * ihs);
				_hist[x][y]++;
			}
		}
	}

	// remove the number 
	int num_removes  = num_chsbd - m_num_calib_chsbd;
	while(num_removes > 0){
		// scoring and find max score chsbd
		s_chsbd_score cs_max;

		cs_max.idx = -1;
		cs_max.score = 0;
		for(int j = 0; j < num_chsbd; j++){
			s_chsbd_score & _cs = cs[j];
			_cs.score = 0.;
			if(_cs.idx == -1)
				continue;

			for(int i = 0; i < num_chsbd_per_frm; i++){
				vector<vector<int>> & _hist = hist[i];
				vector<vector<Point2f>> & _pts = *pts[i];
				vector<Point2f> & _chsbd = _pts[j];
				for(int k = 0; k < _chsbd.size(); k++){
					Point2f & pt = _chsbd[k];
					int x = (int) (pt.x * iws);
					int y = (int) (pt.y * ihs);
					_cs.score += (float)_hist[x][y];					
				}
			}
			if(cs_max.score < _cs.score)
				cs_max = _cs;
		}

		// removing and revoting phase		
		for(int i = 0; i < num_chsbd_per_frm; i++){
			vector<vector<int>> & _hist = hist[i];
			vector<vector<Point2f>> & _pts = *pts[i];

			int j = cs_max.idx;
			vector<Point2f> & _chsbd = _pts[j];
			s_chsbd_score & _cs = cs[j];
			_cs.idx = -1;

			// reducing hist count
			for(int k = 0; k < _chsbd.size(); k++){
				Point2f & pt = _chsbd[k];
				int x = (int) (pt.x * iws);
				int y = (int) (pt.y * ihs);
				_hist[x][y]--;					
			}

			// clear chessboard
			_chsbd.clear();
		}

		num_removes--;
	}

	// remove chessboard entry removed
	for(int i = 0; i < num_chsbd_per_frm; i++){
		vector<vector<Point2f>> & _pts = *pts[i];
		for(vector<vector<Point2f>>::iterator itr = _pts.begin(); 
			itr != _pts.end();)
		{
			if((*itr).size() == 0)
				itr = _pts.erase(itr);
			else
				itr++;
		}
	}

	switch(icam){
	case 0:
		m_num_chsbdl = min(m_num_calib_chsbd, m_num_chsbdl);
		break;
	case 1:
		m_num_chsbdr = min(m_num_calib_chsbd, m_num_chsbdr);
		break;
	case 2:
		m_num_chsbd_com = min(m_num_calib_chsbd, m_num_chsbd_com);
		break;
	default:
		return;
	}

}

void f_glfw_stereo_view::calibrate(int icam)
{
	AWSCamPar * pcp = NULL;
	vector<vector<Point2f>> * ppts = NULL;
	vector<Mat> tvecs;
	vector<Mat> rvecs;
	bool * pbcp;
	Mat * pimg = NULL, * R = NULL, * P = NULL, * map1 = NULL, * map2 = NULL;
	if(icam == 0){
		pbcp = &m_bcpl;
		pcp = &m_camparl;
		ppts = &m_pts_chsbdl;
		pimg = &m_img1;
		P = &m_Pl;
		R = &m_Rl;
		map1 = &m_mapl1;
		map2 = &m_mapl2;
	}else{
		pbcp = &m_bcpr;
		pcp = &m_camparr;
		ppts = &m_pts_chsbdr;
		pimg = &m_img2;
		P = &m_Pr;
		R = &m_Rr;
		map1 = &m_mapr1;
		map2 = &m_mapr2;
	}

	Size sz(pimg->cols, pimg->rows);
	vector<vector<Point3f>> pt3ds;
	pt3ds.resize(ppts->size());
	for(int i = 0; i < ppts->size(); i++)
		pt3ds[i] = m_chsbd.pts;

	if(m_bfisheye){
		pcp->setFishEye(true);
		int flag = 0;
		Mat K, D;
		if(m_bfix_int)
			flag |= fisheye::CALIB_FIX_INTRINSIC;
		if(m_bguess_int)
			flag |= fisheye::CALIB_USE_INTRINSIC_GUESS;
		if(m_bfix_k1)
			flag |= fisheye::CALIB_FIX_K1;
		if(m_bfix_k2)
			flag |= fisheye::CALIB_FIX_K2;
		if(m_bfix_k3)
			flag |= fisheye::CALIB_FIX_K3;
		if(m_bfix_k4)
			flag |= fisheye::CALIB_FIX_K4;

		fisheye::calibrate(pt3ds, *ppts, sz, K, D, rvecs, tvecs, flag);
		pcp->setCvDist(D);
		pcp->setCvPrj(K);
	}else{
		Mat K, D;
		int flag = 0;
		pcp->setFishEye(false);
		if(m_bfix_k1)
			flag |= CV_CALIB_FIX_K1;
		if(m_bfix_k2)
			flag |= CV_CALIB_FIX_K2;
		if(m_bfix_k3)
			flag |= CV_CALIB_FIX_K3;
		if(m_bfix_k4)
			flag |= CV_CALIB_FIX_K4;
		if(m_bfix_k5)
			flag |= CV_CALIB_FIX_K5;
		if(m_bfix_k6)
			flag |= CV_CALIB_FIX_K6;
		if(m_bfix_pp)
			flag |= CV_CALIB_FIX_PRINCIPAL_POINT;
		if(m_brat_mdl)
			flag |= CV_CALIB_RATIONAL_MODEL;
		if(m_bzr_tng)
			flag |= CV_CALIB_ZERO_TANGENT_DIST;
		if(m_bguess_int)
			flag |= CV_CALIB_USE_INTRINSIC_GUESS;
		if(m_bfix_ar)
			flag |= CV_CALIB_FIX_ASPECT_RATIO;

		calibrateCamera(pt3ds, *ppts, sz, K, 
			D, rvecs, tvecs, flag);
		pcp->setCvDist(D);
		pcp->setCvPrj(K);
	}
	init_undistort(*pcp, sz, *R, *P, *map1, *map2, *pbcp);
}

void f_glfw_stereo_view::init_undistort(AWSCamPar & par, Size & sz, 
										Mat & R, Mat & P, Mat & map1, Mat & map2, bool & bcp)
{
	if(par.isFishEye()){		
		Mat K, D;
		K = par.getCvPrjMat().clone();
		D = par.getCvDistFishEyeMat().clone();
		cout << "K" << K << endl;
		cout << "D" << D << endl;
		fisheye::estimateNewCameraMatrixForUndistortRectify(K, D, sz, R, P);
		fisheye::initUndistortRectifyMap(K, D, R, P, sz, CV_16SC2, map1, map2);
		bcp = true;
	}else{
		Mat K, D;
		K = par.getCvPrjMat().clone();
		D = par.getCvDistMat().clone();
		cout << "K" << K << endl;
		cout << "D" << D << endl;
		P = getOptimalNewCameraMatrix(K, D, sz, 1.);
		initUndistortRectifyMap(K, D, R, P, sz, CV_16SC2, map1, map2);
		bcp = true;
	}
	cout << "R" << R << endl;
	cout << "P" << P << endl;
}

void f_glfw_stereo_view::draw_chsbd(const float r, const float g, const float b,
									const float xscale, const float yscale,
									const float xorg, const float yorg, 
									const float w, const float h, 
									vector<vector<Point2f>> & chsbds, const int num_chsbds)
{
	for(int ichsbd = 0; ichsbd < num_chsbds; ichsbd++){
		drawCvChessboard(xscale, yscale, xorg, yorg, w, h, chsbds[ichsbd], r, g, b, 1., 1, 1);
	}
}

//////////////////////////////////////////////////////////////////////////////// f_glfw_claib members
f_glfw_calib::	f_glfw_calib(const char * name):f_glfw_window(name), 	
	m_bcalib_use_intrinsic_guess(false), m_bcalib_fix_campar(false), m_bcalib_fix_focus(false),
	m_bcalib_fix_principal_point(false), m_bcalib_fix_aspect_ratio(false), m_bcalib_zero_tangent_dist(true),
	m_bcalib_fix_k1(true), m_bcalib_fix_k2(true), m_bcalib_fix_k3(true),
	m_bcalib_fix_k4(true), m_bcalib_fix_k5(true), m_bcalib_fix_k6(true), m_bcalib_rational_model(false), 
	m_bFishEye(false), m_bcalib_recompute_extrinsic(false), m_bcalib_check_cond(false),
	m_bcalib_fix_skew(false), m_bcalib_fix_intrinsic(false),
	m_bcalib_done(false), m_num_chsbds(30), m_bthact(false), m_hist_grid(10, 10), m_rep_avg(1.0),
	m_vm(VM_CAM), 
	m_bshow_chsbd_all(true), m_bshow_chsbd_sel(true), m_sel_chsbd(-1), m_bshow_chsbd_density(true),
	m_bsave(false), m_bload(false), m_bdel(false), m_bdet(true), m_bcalib(true),
	m_vb_cam(GL_INVALID_VALUE), m_ib_cam(GL_INVALID_VALUE)
{
	m_fcampar[0] = '\0';

	register_fpar("fchsbd", m_model_chsbd.fname, 1024, "File path for the chessboard model.");
	register_fpar("nchsbd", &m_num_chsbds, "Number of chessboards stocked.");
	register_fpar("fcampar", m_fcampar, "File path of the camera parameter.");
	register_fpar("Wgrid", &m_hist_grid.width, "Number of horizontal grid of the chessboard histogram.");
	register_fpar("Hgrid", &m_hist_grid.height, "Number of vertical grid of the chessboard histogram.");

	// normal camera model
	register_fpar("use_intrinsic_guess", &m_bcalib_use_intrinsic_guess, "Use intrinsic guess.");
	register_fpar("fix_campar", &m_bcalib_fix_campar, "Fix camera parameters");
	register_fpar("fix_focus", &m_bcalib_fix_focus, "Fix camera's focal length");
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
	register_fpar("fx", (m_par.getCvPrj() + 0), "Focal length in x");
	register_fpar("fy", (m_par.getCvPrj() + 1), "Focal length in y");
	register_fpar("cx", (m_par.getCvPrj() + 2), "Principal point in x");
	register_fpar("cy", (m_par.getCvPrj() + 3), "Principal point in y");
	register_fpar("k1", (m_par.getCvDist() + 0), "k1");
	register_fpar("k2", (m_par.getCvDist() + 1), "k2");
	register_fpar("p1", (m_par.getCvDist() + 2), "p1");
	register_fpar("p2", (m_par.getCvDist() + 3), "p2");
	register_fpar("k3", (m_par.getCvDist() + 4), "k3");
	register_fpar("k4", (m_par.getCvDist() + 5), "k4");
	register_fpar("k5", (m_par.getCvDist() + 6), "k5");
	register_fpar("k6", (m_par.getCvDist() + 7), "k6");

	// fisheye camera model
	register_fpar("fisheye", &m_bFishEye, "Yes, use fisheye model.");
	register_fpar("recomp_ext", &m_bcalib_recompute_extrinsic, "Recompute extrinsic parameter.");
	register_fpar("chk_cond", &m_bcalib_check_cond, "Check condition.");
	register_fpar("fix_skew", &m_bcalib_fix_skew, "Fix skew.");
	register_fpar("fix_int", &m_bcalib_fix_intrinsic, "Fix intrinsic parameters.");
	register_fpar("fek1", (m_par.getCvDistFishEye() + 0), "k1 for fisheye");
	register_fpar("fek2", (m_par.getCvDistFishEye() + 1), "k2 for fisheye");
	register_fpar("fek3", (m_par.getCvDistFishEye() + 2), "k3 for fisheye");
	register_fpar("fek4", (m_par.getCvDistFishEye() + 3), "k4 for fisheye");

	pthread_mutex_init(&m_mtx, NULL);
}

f_glfw_calib::~f_glfw_calib()
{
}

bool f_glfw_calib::init_run()
{
	if(m_chin.size() == 0)
		return false;

	m_pin = dynamic_cast<ch_image*>(m_chin[0]);
	if(m_pin == NULL)
		return false;

	if(!f_glfw_window::init_run())
		return false;

	if(!m_model_chsbd.load()){
		cerr << "Failed to setup chessboard model with the model file " << m_model_chsbd.fname << endl;
		return false;
	}

	m_objs.resize(m_num_chsbds, NULL);
	m_score.resize(m_num_chsbds);
	m_num_chsbds_det = 0;

	m_dist_chsbd = Mat::zeros(m_hist_grid.height, m_hist_grid.width, CV_32SC1);

	m_par.setFishEye(m_bFishEye);

	// initialize camera model's color as transparent gray
	for(int i = 0; i < 16; i++){
		m_cam[i].r = m_cam[i].g = m_cam[i].b = 0.5;
		m_cam[i].a = 0.25;
		m_cam[i].nx = m_cam[i].ny = m_cam[i].nz = 0.;
		m_cam[i].x = m_cam[i].y = m_cam[i].z = 0.;
	}

	for(int i = 0; i < 15; i++){
		m_cam_idx[i] = i;
	}
	m_cam_idx[15] = 14;
	m_cam_idx[16] = 15;
	m_cam_idx[17] = 12;

	return true;
}

bool f_glfw_calib::proc()
{
	if(glfwWindowShouldClose(pwin()))
		return false;

	long long timg;
	Mat img = m_pin->get_img(timg);
	if(img.empty())
		return true;
	if(m_timg == timg)
		return true;
	
	{
		// debugging code. I doubt that the window size specified in glfwCreateWindow is not 
		// the same as the size of the frame buffer.
		int w, h;
		glfwGetFramebufferSize(pwin(), &w, &h);
		if(m_sz_win.width != w || m_sz_win.height){
			cerr << "Frame buffer size is different from given as the filter's parameter." << endl;
		}
	}
	
	// if the detecting thread is not active, convert the image into a grayscale one, and invoke detection thread.
	if(!m_bthact){
		cvtColor(img, m_img_det, CV_RGB2GRAY);
		pthread_create(&m_thdet, NULL, thdet, (void*) this);
	}

	if(!m_bsave){
		pthread_mutex_lock(&m_mtx);
		if(m_fcampar[0] == '\0' || !m_par.write(m_fcampar)){
			cerr << "Failed to save camera parameters into " << m_fcampar << endl;
		}
		pthread_mutex_unlock(&m_mtx);
		m_bsave = false;
	}

	if(!m_bload){
		pthread_mutex_lock(&m_mtx);
		if(m_fcampar[0] == '\0' || !m_par.read(m_fcampar)){
			cerr << "Failed to load camera parameters from " << m_fcampar << endl;
		}
		pthread_mutex_unlock(&m_mtx);
		m_bload = false;
	}

	if(m_bdel){
		if(m_sel_chsbd >= 0 && m_sel_chsbd < m_objs.size()){
			pthread_mutex_lock(&m_mtx);
			delete [] m_objs[m_sel_chsbd];
			m_score[m_sel_chsbd] = s_chsbd_score();
			for(;m_sel_chsbd >= 0; m_sel_chsbd--)
				if(m_objs[m_sel_chsbd])
					break;
			m_num_chsbds_det--;
			pthread_mutex_unlock(&m_mtx);
		}
	}

	m_timg = timg;

	if(m_vm == VM_CAM){
		glRasterPos2i(-1, -1);
		cnvCVBGR8toGLRGB8(img);
		glDrawPixels(img.cols, img.rows, GL_RGB, GL_UNSIGNED_BYTE, img.data);

		// For 2D rendering mode, the origin is set at left-bottom as (0, 0), clipping rectangle has the 
		// width and height the same as the frame buffer size.
		glViewport(0, 0, m_sz_win.width, m_sz_win.height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1., -1., -1., 1., -1., 1.);		

		// overlay chessboard if needed
		if(m_bshow_chsbd_all){
			for(int iobj = 0; iobj < m_objs.size(); iobj++){
				drawCvChessboard(m_sz_win, m_objs[iobj]->pt2d, 1.0, 0.0, 0.0, 0.5, 3, 1);
			}
		}

		if(m_bshow_chsbd_sel){
			if(m_sel_chsbd < 0 || m_sel_chsbd >= m_objs.size()){
				m_sel_chsbd = (int) m_objs.size() - 1;
			}else{
				drawCvChessboard(m_sz_win, m_objs[m_sel_chsbd]->pt2d, 1.0, 0.0, 0.0, 1.0, 3, 2);
			}
		}

		// overlay point density if needed
		if(m_bshow_chsbd_density){
			drawCvPointDensity(m_dist_chsbd, m_num_chsbds, m_hist_grid, 1.0, 0., 0., 128, 0);
		}

		// overlay information (Number of chessboard, maximum, minimum, average scores, reprojection error)
		float hfont = (float)(24. / (float) img.rows);
		float wfont = (float)(24. / (float) img.cols);
		float x = wfont;
		float y = hfont;
		char buf[256];

		// calculating chessboard's stats
		double smax = 0., smin = DBL_MAX, savg = 0.;
		for(int i = 0; i < m_num_chsbds; i++){
			if(!m_objs[i])
				continue;

			double s = m_score[i].tot;
			smax = max(smax, s);
			smin = min(smin, s);
			savg += s;
		}

		savg /= (double) m_num_chsbds_det;

		snprintf(buf, 255, "Nchsbd= %d/%d, Smax=%f Smin=%f Savg=%f Erep=%f ",
			m_num_chsbds_det, m_num_chsbds_det, smax, smin, savg, m_rep_avg);
		drawGlText(x, y, buf, 0, 1, 0, 1, GLUT_BITMAP_TIMES_ROMAN_24);
		y+= hfont;
		
		// indicating infromation about selected chessboard.
		if(m_bshow_chsbd_sel && m_sel_chsbd >= 0 && m_sel_chsbd < m_num_chsbds && m_objs[m_sel_chsbd]){
			snprintf(buf, 255, "Chessboard %d", m_sel_chsbd);
			drawGlText(x, y, buf, 0, 1, 0, 1, GLUT_BITMAP_TIMES_ROMAN_24);
			y+= hfont;

			snprintf(buf, 255, "Score: %f (Crn: %f Sz %f Angl %f Erep %f)", 
				m_score[m_sel_chsbd].tot, m_score[m_sel_chsbd].crn, m_score[m_sel_chsbd].sz, 
				m_score[m_sel_chsbd].angl, m_score[m_sel_chsbd].rep);
			drawGlText(x, y, buf, 0, 1, 0, 1, GLUT_BITMAP_TIMES_ROMAN_24);
			y+= hfont;
		}
	}else{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(m_bcalib){
			// setting projection matrix (Basically setting it the same as the camera parameter.)
			glViewport(0, 0, m_sz_win.width, m_sz_win.height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			double * pP = m_par.getCvPrj();

			double fx = pP[0], fy = pP[1], cx = pP[2], cy = pP[3];
			double w = m_img_det.cols, h = m_img_det.rows;
			double left = cx / w, right = (1.0 - left);
			double top = cy / h, bottom = (1.0 - top);

			// the scene depth is set as 0.1 to 100 meter
			glFrustum(-cx, w - cx, -h + cy, cy, 0.1, 100.);

			GLfloat mobj[16];

			// setting model-view matrix chessboards and camera
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(m_trx, 0., m_trz);
			glRotatef(m_thx, 1.0, 0., 0.);
			glRotatef(m_thy, 0., 1., 0.);
			for(int i = 0; i < m_num_chsbds; i++){				
				if(!m_objs[i])
					continue;

				glPushMatrix();
				Mat R = Mat(3, 3, CV_64FC1);
				// object transformation
				exp_so3(m_objs[i]->rvec.ptr<double>(), R.ptr<double>());
				reformRtAsGl4x4(R, m_objs[i]->tvec, mobj);
				// multiply cg-cam motion 
				glMultMatrixf(mobj);

				glPointSize(3);
				glBegin(GL_POINTS);
				for(int ipt = 0; ipt < m_model_chsbd.pts.size(); ipt++){
					Point3f & pt = m_model_chsbd.pts[ipt];
					glColor4f(1.0, 0, 0, 1.0);
					glVertex3f(pt.x, pt.y, pt.z);
				}
				glEnd();
				glPopMatrix();
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glBindBuffer(GL_ARRAY_BUFFER, m_vb_cam);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib_cam);
			glInterleavedArrays(GL_C4F_N3F_V3F, 0, NULL);
			glDrawElements(GL_TRIANGLES, sizeof(m_cam_idx), GL_UNSIGNED_INT, NULL);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		}else{// camera parameters have not been fixed yet. we cant render the objects.
		}
	}
	glfwSwapBuffers(pwin());
	glfwPollEvents();

	return true;
}

void * f_glfw_calib::thdet(void * ptr)
{
	f_glfw_calib * pclb = (f_glfw_calib*) ptr;
	return pclb->_thdet();
}

void * f_glfw_calib::_thdet()
{
	if(m_hist_grid.width != m_dist_chsbd.cols || 
		m_hist_grid.height != m_dist_chsbd.rows)
		refresh_chsbd_dist();

	if(m_bdet){
		s_obj * pobj = m_model_chsbd.detect(m_img_det);
		if(!pobj) 
			return NULL;

		// calculating score of the chessboard.
		s_chsbd_score sc;
		sc.rep = 1.0;
		sc.crn = calc_corner_contrast_score(m_img_det, pobj->pt2d);
		calc_size_and_angle_score(pobj->pt2d, sc.sz, sc.angl);

		calc_tot_score(sc);

		// replacing the chessboard if possible.
		bool is_changed = false;
		for(int i = 0; i < m_num_chsbds; i++){
			if(m_objs[i] == NULL){
				pthread_mutex_lock(&m_mtx);
				m_objs[i] = pobj;
				m_score[i] = sc;
				add_chsbd_dist(pobj->pt2d);
				is_changed = true;
				m_num_chsbds++;
				pthread_mutex_unlock(&m_mtx);
				break;
			}

			if(m_num_chsbds_det == m_num_chsbds && m_score[i].tot < sc.tot){
				pthread_mutex_lock(&m_mtx);
				s_obj * ptmp = pobj;
				pobj = m_objs[i];
				m_objs[i] = ptmp;
				sub_chsbd_dist(pobj->pt2d);
				add_chsbd_dist(ptmp->pt2d);
				pthread_mutex_unlock(&m_mtx);
				delete pobj;
				is_changed = true;
				break;
			}
		}

		if(is_changed)
			recalc_chsbd_dist_score();
	}

	if(m_bcalib && m_num_chsbds == m_num_chsbds_det){
		calibrate();
		// after calibration camera object
		pthread_mutex_lock(&m_mtx);
		resetCameraModel();
		pthread_mutex_unlock(&m_mtx);
	}

	return NULL;
}

// The size score is the area of the largest cell.
// The angle score is the ratio of the area of largest cell and that of the smallest cell.
void f_glfw_calib::calc_size_and_angle_score(vector<Point2f> & pts, double & ssz, double & sangl)
{
	Size sz = Size(m_model_chsbd.par_chsbd.w, m_model_chsbd.par_chsbd.h);
	double amax = 0, amin = DBL_MAX;
	double a;
	Point2f v1, v2;

	int i0, i1, i2;

	// left-top corner
	i0 = 0;
	i1 = 1;
	i2 = sz.width;
	v1 = pts[i1] - pts[i0];
	v2 = pts[i2] - pts[i0];

	// Eventhoug the projected grid is not the parallelogram, we now approximate it as a parallelogram.
	a = abs(v1.x * v2.y - v1.y * v2.x); 
	amax = max(a, amax);
	amin = min(a, amin);

	// left-bottom corner
	i0 = sz.width * (sz.height - 1);
	i1 = i0 + 1;
	i2 = i0 - sz.width;
	v1 = pts[i1] - pts[i0];
	v2 = pts[i2] - pts[i0];
	a = abs(v1.x * v2.y - v1.y * v2.x);
	amax = max(a, amax);
	amin = min(a, amin);

	// right-top corner
	i0 = sz.width - 1;
	i1 = i0 - 1;
	i2 = i0 + sz.width;
	v1 = pts[i1] - pts[i0];
	v2 = pts[i2] - pts[i0];
	a = abs(v1.x * v2.y - v1.y * v2.x);
	amax = max(a, amax);
	amin = min(a, amin);

	// right-bottom corner
	i0 = sz.width * sz.height - 1;
	i1 = i0 - 1;
	i2 = i0 - sz.width;
	v1 = pts[i1] - pts[i0];
	v2 = pts[i2] - pts[i0];
	a = abs(v1.x * v2.y - v1.y * v2.x);
	amax = max(a, amax);
	amin = min(a, amin);
	ssz = amax;
	sangl = amax / amin;
}

// calculating average of contrasts of pixels around chessboard's points.
// the contrast of a point is calculated (Imax-Imin) / (Imax+Imin) for pixels around the point
double f_glfw_calib::calc_corner_contrast_score(Mat & img, vector<Point2f> & pts)
{
	uchar * pix = img.data;
	uchar vmax = 0, vmin = 255;
	double csum = 0.;
	//Note that the contrast of the point is calculated in 5x5 image patch centered at the point. 
	
	int org = - 2 * img.cols - 2;
	for(int i = 0; i < pts.size(); i++){
		Point2f & pt = pts[i];
		
		pix = img.data + (int) (img.cols * pt.y + pt.x + 0.5) + org;
		for(int y = 0; y < 5; y++){
			for(int x = 0; x < 5; x++, pix++){
				vmax = max(vmax, *pix);
				vmin = min(vmin, *pix);
			}
			pix += org - 5;
		}
		csum += (double)(vmax - vmin) / (double)((int)vmax + (int)vmin);
	}

	return csum / (double) pts.size();
}

void f_glfw_calib::add_chsbd_dist(vector<Point2f> & pts)
{
	double wstep = m_img_det.cols / m_hist_grid.width;
	double hstep = m_img_det.rows / m_hist_grid.height;
	for(int i = 0; i < pts.size(); i++){
		int x, y;
		x = (int) (pts[i].x / wstep);
		y = (int) (pts[i].y / hstep);
		m_dist_chsbd.at<int>(x, y) += 1;
	}
}

void f_glfw_calib::sub_chsbd_dist(vector<Point2f> & pts)
{
	double wstep = m_img_det.cols / m_hist_grid.width;
	double hstep = m_img_det.rows / m_hist_grid.height;
	for(int i = 0; i < pts.size(); i++){
		int x, y;
		x = (int) (pts[i].x / wstep);
		y = (int) (pts[i].y / hstep);
		m_dist_chsbd.at<int>(x, y) -= 1;
	}
}

void f_glfw_calib::refresh_chsbd_dist()
{
	m_dist_chsbd = Mat::zeros(m_hist_grid.height, m_hist_grid.width, CV_32SC1);
	for(int iobj = 0; iobj < m_objs.size(); iobj++){
		s_obj * pobj = m_objs[iobj];
		add_chsbd_dist(pobj->pt2d);
	}
}

double f_glfw_calib::calc_chsbd_dist_score(vector<Point2f> & pts)
{
	double wstep = m_img_det.cols / m_hist_grid.width;
	double hstep = m_img_det.rows / m_hist_grid.height;
	int sum = 0;
	for(int i = 0; i < pts.size(); i++){
		int x, y;
		x = (int) (pts[i].x / wstep);
		y = (int) (pts[i].y / hstep);
		sum += m_dist_chsbd.at<int>(x, y);
	}
	return 1.0 / ((double) sum + 0.001);
}

void f_glfw_calib::recalc_chsbd_dist_score()
{
	m_dist_chsbd = Mat::zeros(m_hist_grid.height, m_hist_grid.width, CV_32SC1);
	for(int iobj = 0; iobj < m_objs.size(); iobj++){
		s_obj * pobj = m_objs[iobj];
		calc_chsbd_dist_score(pobj->pt2d);
	}
}

int f_glfw_calib::gen_calib_flag()
{
	int flag = 0;
	if(m_bFishEye){
		flag |= (m_bcalib_recompute_extrinsic ? fisheye::CALIB_RECOMPUTE_EXTRINSIC : 0);
		flag |= (m_bcalib_check_cond ? fisheye::CALIB_CHECK_COND : 0);
		flag |= (m_bcalib_fix_skew ? fisheye::CALIB_FIX_SKEW : 0);
		flag |= (m_bcalib_fix_intrinsic ? fisheye::CALIB_FIX_INTRINSIC : 0);
		flag |= (m_bcalib_fix_k1 ? fisheye::CALIB_FIX_K1 : 0);
		flag |= (m_bcalib_fix_k2 ? fisheye::CALIB_FIX_K2 : 0);
		flag |= (m_bcalib_fix_k3 ? fisheye::CALIB_FIX_K3 : 0);
		flag |= (m_bcalib_fix_k4 ? fisheye::CALIB_FIX_K4 : 0);
	}else{
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
	}
	return flag;
}

void f_glfw_calib::calibrate()
{
	m_par.setFishEye(m_bFishEye);

	Size sz_chsbd(m_model_chsbd.par_chsbd.w, m_model_chsbd.par_chsbd.h);
	int num_pts = sz_chsbd.width * sz_chsbd.height;

	vector<Mat> pt2ds;
	vector<Mat> pt3ds;
	vector<Mat> rvecs;
	vector<Mat> tvecs;

	int flags = gen_calib_flag();

	pt2ds.resize(m_num_chsbds_det);
	pt3ds.resize(m_num_chsbds_det);
	for(int iobj = 0; iobj < m_objs.size(); iobj++){
		if(m_objs[iobj] == NULL)
			continue;
		pt2ds[iobj] = Mat(m_objs[iobj]->pt2d, false);
		pt3ds[iobj] = Mat(m_model_chsbd.pts, false);
	}
	
	Mat P, D;
	if(m_bFishEye){
		m_Erep = fisheye::calibrate(pt3ds, pt2ds, sz_chsbd, P, D,
			rvecs, tvecs, flags);
	}else{
		m_Erep = calibrateCamera(pt3ds, pt2ds, sz_chsbd,
			P, D, rvecs, tvecs, flags);
	}

	// copying obtained camera parameter to m_par
	pthread_mutex_lock(&m_mtx);
	m_par.setCvPrj(P);
	m_par.setCvDist(D);
	pthread_mutex_unlock(&m_mtx);

	m_bcalib_done = true;

	// calculating reprojection error 
	double rep_tot = 0.;
	
	for(int iobj = 0, icount = 0; iobj < m_objs.size(); iobj++){
		if(m_objs[iobj] == NULL)
			continue;

		m_objs[iobj]->rvec = rvecs[icount];
		m_objs[iobj]->tvec = tvecs[icount];
		vector<Point2f> & pt2dprj = m_objs[iobj]->pt2dprj;
		vector<Point2f> & pt2d = m_objs[iobj]->pt2d;

		if(m_bFishEye){
			fisheye::projectPoints(m_model_chsbd.pts, pt2dprj, rvecs[icount], tvecs[icount], 
				m_par.getCvPrjMat(), m_par.getCvDistFishEyeMat());
		}else{
			prjPts(m_model_chsbd.pts, pt2dprj,
				m_par.getCvPrjMat(), m_par.getCvDistMat(),
				rvecs[icount], tvecs[icount]);
		}
		double sum_rep = 0.;
		for(int ipt = 0; ipt < pt2d.size(); ipt++){
			Point2f & ptprj = pt2dprj[ipt];
			Point2f & pt = pt2d[ipt];
			Point2f ptdiff = pt - ptprj;
			double rep = ptdiff.x * ptdiff.x + ptdiff.y * ptdiff.y;
			sum_rep += rep;
		}
		rep_tot += sum_rep;
		m_score[iobj].rep = sqrt(sum_rep / (double) num_pts);
	}

	m_rep_avg = rep_tot / (double)(num_pts * m_num_chsbds);
}

void f_glfw_calib::resetCameraModel(){

	if(m_ib_cam != GL_INVALID_VALUE || m_vb_cam != GL_INVALID_VALUE){
		// if the buffers have already been made, delete them.
		glDeleteBuffers(1, &m_ib_cam);
		glDeleteBuffers(1, &m_vb_cam);
		m_ib_cam = GL_INVALID_VALUE;
		m_vb_cam = GL_INVALID_VALUE;
	}

	double * pP = m_par.getCvPrj();
	double fx = pP[0], fy = pP[1], cx = pP[2], cy = pP[3];
	double w = m_img_det.cols, h = m_img_det.rows;
	float sx = 0.1f, sy = (float)(sx * h / w);
	float left = (float)(cx / w), right = (float)(1.0 - left);
	float top =(float)(cy / h), bottom = (float)(1.0 - top);
	left *= sx, right *= sx;
	top *= sy, bottom *= sy;
	float z =  (float)(sx * fx / w);

	float nx, ny, nz;
	// setting cam's focal point.
	for(int i = 0; i < 12; i += 3){
		m_cam[i].x = m_cam[i].y = m_cam[i].z = 0.;
	}

	// top surface vertices and normals
	m_cam[2].x = -left, m_cam[2].y = top, m_cam[2].z = z;
	m_cam[1].x = right, m_cam[1].y = top, m_cam[1].z = z;
	crossProduct(m_cam[1].x, m_cam[1].y, m_cam[1].z,
		m_cam[2].x, m_cam[2].y, m_cam[2].z,
		nx, ny, nz);
	for(int i = 0; i < 3; i++){
		m_cam[i].nx = nx;
		m_cam[i].ny = ny;
		m_cam[i].nz = nz;
	}

	// bottom 
	m_cam[4].x = -left, m_cam[4].y = -bottom, m_cam[4].z = z;
	m_cam[5].x = right, m_cam[5].y = -bottom, m_cam[5].z = z;
	crossProduct(m_cam[4].x, m_cam[4].y, m_cam[4].z,
		m_cam[5].x, m_cam[5].y, m_cam[5].z,
		nx, ny, nz);
	normalize(nx, ny, nz);
	for(int i = 3; i < 6; i++){
		m_cam[i].nx = nx;
		m_cam[i].ny = ny;
		m_cam[i].nz = nz;
	}

	// left
	m_cam[8].x = -left, m_cam[8].y = -bottom, m_cam[8].z = z;
	m_cam[7].x = -left, m_cam[7].y = top, m_cam[7].z = z;
	crossProduct(m_cam[7].x, m_cam[7].y, m_cam[7].z,
		m_cam[8].x, m_cam[8].y, m_cam[8].z,
		nx, ny, nz);
	normalize(nx, ny, nz);
	for(int i = 6; i < 9; i++){
		m_cam[i].nx = nx;
		m_cam[i].ny = ny;
		m_cam[i].nz = nz;
	}

	// right
	m_cam[10].x = right, m_cam[10].y = -bottom, m_cam[10].z = z;
	m_cam[11].x = right, m_cam[11].y = top, m_cam[11].z = z;
	crossProduct(m_cam[10].x, m_cam[10].y, m_cam[10].z,
		m_cam[11].x, m_cam[11].y, m_cam[11].z,
		nx, ny, nz);
	normalize(nx, ny, nz);
	for(int i = 9; i < 12; i++){
		m_cam[i].nx = nx;
		m_cam[i].ny = ny;
		m_cam[i].nz = nz;
	}

	// sensor surface
	m_cam[12].x = -left, m_cam[12].y = -bottom, m_cam[12].z = z;
	m_cam[13].x = -left, m_cam[13].y = top, m_cam[13].z = z;
	m_cam[14].x = right, m_cam[14].y = top, m_cam[14].z = z;
	m_cam[15].x = right, m_cam[15].y = -bottom, m_cam[15].z = z;
	nx = ny = 0.;
	nz = 1.0;
	for(int i = 12; i < 16; i++){
		m_cam[i].nx = nx;
		m_cam[i].ny = ny;
		m_cam[i].nz = nz;
	}

	glGenBuffers(1, &m_vb_cam);
	glBindBuffer(GL_ARRAY_BUFFER, m_vb_cam);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(AWS_VERTEX), m_cam, GL_STATIC_DRAW);

	glGenBuffers(1, &m_ib_cam);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib_cam);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 18 * sizeof(GLuint), m_cam_idx, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void f_glfw_calib::_key_callback(int key, int scancode, int action, int mods)
{
	switch(key){
	case GLFW_KEY_A:
	case GLFW_KEY_B:
	case GLFW_KEY_C:
		m_bcalib = !m_bcalib;
		break;
	case GLFW_KEY_D:
		m_bdet = !m_bdet;
		break;
	case GLFW_KEY_E:
	case GLFW_KEY_F:
	case GLFW_KEY_G:
	case GLFW_KEY_H:
	case GLFW_KEY_I:
	case GLFW_KEY_J:
	case GLFW_KEY_K:
	case GLFW_KEY_L:
		if(mods & GLFW_MOD_CONTROL){
			m_bload = true;
		}
		break;
	case GLFW_KEY_M:
	case GLFW_KEY_N:
	case GLFW_KEY_O:
	case GLFW_KEY_P:
	case GLFW_KEY_Q:
	case GLFW_KEY_R:
		if(m_vm == VM_CAM){
			break;
		}else{
			m_thx = m_thy = 0.;
			m_trx = m_trz = 0.;
		}
		break;
	case GLFW_KEY_S:
		if(mods & GLFW_MOD_CONTROL){
			m_bsave = true;
		}
		break;
	case GLFW_KEY_T:
	case GLFW_KEY_U:
	case GLFW_KEY_V:
		if(m_vm == VM_CAM){
			m_vm = VM_CG;
		}else{
			m_vm = VM_CAM;
		}
		break;
	case GLFW_KEY_W:
	case GLFW_KEY_X:
	case GLFW_KEY_Y:
	case GLFW_KEY_Z:
	case GLFW_KEY_RIGHT:
		if(m_vm == VM_CAM)
			m_sel_chsbd = (m_sel_chsbd + 1) % (int) m_objs.size();
		else{
			if(mods & GLFW_MOD_CONTROL){
				m_thy += 5.0;
				if(m_thy >= 360.0)
					m_thy -= 360.;
			}else if(mods & GLFW_MOD_SHIFT){
				m_trx += 1.0;
			}
		}
		break;
	case GLFW_KEY_LEFT:
		if(m_vm == VM_CAM){
			m_sel_chsbd = m_sel_chsbd - 1;
			if(m_sel_chsbd < 0)
				m_sel_chsbd += (int) m_objs.size();
		}else{
			if(mods & GLFW_MOD_CONTROL){
				m_thy -= 5.0;
				if(m_thy < 0.)
					m_thy += 360.;
			}else if(mods & GLFW_MOD_SHIFT){
				m_trx -= 1.0;
			}
		}
		break;
	case GLFW_KEY_UP:
		if(m_vm == VM_CAM){
			break;
		}else{
			if(mods & GLFW_MOD_CONTROL){
				m_thx += 5.0;
				if(m_thx >= 360.0)
					m_thx -= 360.;
			}else if(mods & GLFW_MOD_SHIFT){
				m_trz += 1.;
			}
		}
		break;
	case GLFW_KEY_DOWN:
		if(m_vm == VM_CAM){
			break;
		}else{
			if(mods & GLFW_MOD_CONTROL){
				m_thx -= 5.0;
				if(m_thx < 0.)
					m_thx += 360.;
			}else if(mods & GLFW_MOD_SHIFT){
				m_trz = -1.;
			}
		}
		break;
	case GLFW_KEY_SPACE:
	case GLFW_KEY_BACKSPACE:
	case GLFW_KEY_ENTER:
	case GLFW_KEY_DELETE:
		m_bdel = true;
		break;
	case GLFW_KEY_TAB:
	case GLFW_KEY_HOME:
	case GLFW_KEY_END:
	case GLFW_KEY_F1:
		m_bshow_chsbd_all = !m_bshow_chsbd_all;
		break;
	case GLFW_KEY_F2:
		m_bshow_chsbd_sel = !m_bshow_chsbd_sel;
		break;
	case GLFW_KEY_F3:
		m_bshow_chsbd_density = !m_bshow_chsbd_density;
		break;
	case GLFW_KEY_F4:
	case GLFW_KEY_F5:
	case GLFW_KEY_F6:
	case GLFW_KEY_F7:
	case GLFW_KEY_F8:
	case GLFW_KEY_F9:
	case GLFW_KEY_F10:
	case GLFW_KEY_F11:
	case GLFW_KEY_F12:
	case GLFW_KEY_UNKNOWN:
		break;
	}
}

#endif

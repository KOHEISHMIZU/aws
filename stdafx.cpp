// stdafx.cpp : �W���C���N���[�h aws.pch �݂̂�
// �܂ރ\�[�X �t�@�C���́A�v���R���p�C���ς݃w�b�_�[�ɂȂ�܂��B
// stdafx.obj �ɂ̓v���R���p�C���ς݌^��񂪊܂܂�܂��B

#include "stdafx.h"

// TODO: ���̃t�@�C���ł͂Ȃ��ASTDAFX.H �ŕK�v��
// �ǉ��w�b�_�[���Q�Ƃ��Ă��������B

#ifdef _DEBUG

// OpenCV 2.4.10
#pragma comment(lib, "opencv_core2410d.lib")
#pragma comment(lib, "opencv_contrib2410d.lib")
#pragma comment(lib, "opencv_features2d2410d.lib")
#pragma comment(lib, "opencv_legacy2410d.lib")
#pragma comment(lib, "opencv_objdetect2410d.lib")
#pragma comment(lib, "opencv_highgui2410d.lib")
#pragma comment(lib, "opencv_imgproc2410d.lib")
#pragma comment(lib, "opencv_calib3d2410d.lib")
#ifdef SANYO_HD5400
#pragma comment(lib, "libjpegd.lib")
#pragma comment(lib, "libcurld_imp.lib")
#endif
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Quartz.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9d.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")
#else
// OpenCV 2.4.10
#pragma comment(lib, "opencv_core2410.lib")
#pragma comment(lib, "opencv_contrib2410.lib")
#pragma comment(lib, "opencv_features2d2410.lib")
#pragma comment(lib, "opencv_legacy2410.lib")
#pragma comment(lib, "opencv_objdetect2410.lib")
#pragma comment(lib, "opencv_highgui2410.lib")
#pragma comment(lib, "opencv_imgproc2410.lib")
#pragma comment(lib, "opencv_calib3d2410.lib")

#ifdef SANYO_HD5400
#pragma comment(lib, "libcurl_imp.lib")
#pragma comment(lib, "libjpeg.lib")
#endif
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Quartz.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")
#endif

#ifdef AVT_CAM
#pragma comment(lib, "PvAPI.lib")
#endif

#ifdef GLFW_WINDOW
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "glu32.lib")
#ifdef _DEBUG
#pragma comment(lib, "freeglutd.lib")
#else
#pragma comment(lib, "freeglut.lib")
#endif
#endif
// stdafx.cpp : �W���C���N���[�h aws.pch �݂̂�
// �܂ރ\�[�X �t�@�C���́A�v���R���p�C���ς݃w�b�_�[�ɂȂ�܂��B
// stdafx.obj �ɂ̓v���R���p�C���ς݌^��񂪊܂܂�܂��B

#include "stdafx.h"

// TODO: ���̃t�@�C���ł͂Ȃ��ASTDAFX.H �ŕK�v��
// �ǉ��w�b�_�[���Q�Ƃ��Ă��������B

#ifdef _DEBUG

// OpenCV 2.4.2
#pragma comment(lib, "opencv_core249d.lib")
#pragma comment(lib, "opencv_contrib249d.lib")
#pragma comment(lib, "opencv_features2d249d.lib")
#pragma comment(lib, "opencv_legacy249d.lib")
#pragma comment(lib, "opencv_objdetect249d.lib")
#pragma comment(lib, "opencv_highgui249d.lib")
#pragma comment(lib, "opencv_imgproc249d.lib")
#pragma comment(lib, "opencv_calib3d249d.lib")

#pragma comment(lib, "libcurld_imp.lib")
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "libjpegd.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Quartz.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9d.lib")

#else
// OpenCV 2.4.2s
#pragma comment(lib, "opencv_core249.lib")
#pragma comment(lib, "opencv_contrib249.lib")
#pragma comment(lib, "opencv_features2d249.lib")
#pragma comment(lib, "opencv_legacy249.lib")
#pragma comment(lib, "opencv_objdetect249.lib")
#pragma comment(lib, "opencv_highgui249.lib")
#pragma comment(lib, "opencv_imgproc249.lib")
#pragma comment(lib, "opencv_calib3d249.lib")

#pragma comment(lib, "libcurl_imp.lib")
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "libjpeg.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Quartz.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#endif

#pragma comment(lib, "PvAPI.lib");
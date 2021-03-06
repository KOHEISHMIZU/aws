#ifndef _AWS_VOBJ_H_
#define _AWS_VOBJ_H_

// s_edge represents the edge between points defined in the s_model
// This is used for drawing the wire frame model. 
struct s_edge{
	int s, e;
	s_edge():s(0), e(0){};
};

struct s_surface{
	vector<int> pts;
};

// s_part describe the part's structure. translation, rotation axis and the origin relative to the body
// part's nesting is not supported yet. it could be the future issue.
struct s_part{
	vector<int> pts; // point indices of the parts (the points are that in the s_model object contains this object)
	int org; // point index for the rotation origin. 
	Point3f axis; // translation or rotation vector
	bool rot, trn; // translation / rotation flag
	s_part():org(0), axis(0., 0., 0.), rot(true), trn(true){	}
};

///////////////////////////////////////////////////////////////// s_model
// s_model represents a 3D model tracked in the scene by f_inspector.
// It contains points and edges, and their projection method.
#define DEBUG_CHSBDDET

class c_imgalign;
class ModelTrack;

struct s_obj;
struct s_model
{
	char fname[1024]; // path to the model file.
	int ref; // reference counts (Number of object reffering the model)
	string name; // model name

	vector<Point3f> pts; // model points
	vector<s_edge> edges; // model edges
	vector<s_surface> surfaces; // model surface
	vector<s_part> parts; // model parts  

	// temporal data strucuture. Calculated immediately before projection, the data structure should have deformed model points.
	vector<Point3f> pts_deformed; 

	enum e_model_type{
		EMT_NORMAL, EMT_CHSBD, EMT_UNKNOWN
	} type;

	// only used for chessboard model
	struct s_chsbd{
		int w, h; // chessboard width and height
		float p; // chessboard pitch in meter
		bool parse(const char * name, e_model_type & type, vector<Point3f> & pts, vector<s_edge> & edges);
		bool detect(Mat & img, vector<Point2f> & pt2d);
	};

	union{
		s_chsbd par_chsbd;
	};

	static const char * m_str_type[ EMT_UNKNOWN + 1];

	float xmin, ymin, zmin, xmax, ymax, zmax;
	s_model():ref(0), xmin(FLT_MAX), ymin(FLT_MAX), zmin(FLT_MAX),
		xmax(-FLT_MAX), ymax(-FLT_MAX), zmax(-FLT_MAX)
	{
		fname[0] = '\0';
	}

	void calc_bounds();

	int get_num_pts()
	{
		return (int) pts.size();
	}

	// get_max_dist calculates size of the bounding box of the model and	
	// returns its diagonal length.
	double get_max_dist();

	double get_xsize()
	{
		return xmax - xmin;
	}

	double get_ysize()
	{
		return ymax - ymin;
	}

	double get_zsize()
	{
		return zmax - zmin;
	}

	void proj(vector<Point2f> & pt2d, Mat & cam_int, Mat & cam_dist, Mat & rvec_cam, Mat & tvec_cam, 
		Mat & rvec_obj, Mat & tvec_obj);

	bool load(const char * afname);
	bool load();

	s_obj * detect(Mat & img, s_obj * pobj = NULL);
};

///////////////////////////////////////////////////////////////// aws object handling loop
// mapping, localization
// object detection, object classification, object 3d-reconstruction, object tracking, object motion prediction 
//
//


///////////////////////////////////////////////////////////////// s_obj
// Object attributes
// Identifier (name)
// model property (s_model)
//  -3d Vertices with color
//  -Surface index
//  -Texture
//
// dynamic property (time dependent)
// -deformation
// -flag visible points
// -flag visible surface
// -Attitude (R and t)
// -Projected vertices
// -Detected vertices
// -Relative attitude
//   - (x, y, z)
//   - (roll, pitch, yaw)
// -Absolute coordinate
//   -ECEF (x, y, z)
//   -BIH (lon, lat, alt)
// -Velocity
//   -ECEF (vx,vy,vz)


// s_obj represents the object in the scene.
// User can specify its feature points, find the correspondance between the model
// and the points. 
struct s_obj
{
	s_model * pmdl;
	long long t;
	char * name; // Object name ( is basically named as <model name>_<number>) 
	Rect bb2d;
	vector<Point2f> pt2d;
	vector<Point2f> pt2dprj;
	vector<int> visible; // true if 2d point is visible in the image

	vector<Mat> ptx_tmpl; // point's template images.

	bool is_attitude_fixed;

	// Object attitude.
	Mat R;
	Mat tvec, rvec;

	// roll pitch yaw and position relative to the current coordinate
	double roll, pitch, yaw;
	Point3f pos;

	double err_r, err_t;

	vector<double> dpart; // part's deformation value.

	// Update flag. (True if intermidiate parameters have already been calculated)
	bool update;

	Mat jacobian;
	Mat jmax; // maximum values of jacobian for each parameter
	Mat hessian;
	Mat dp;
	Mat err;
	Mat jterr;
	double ssd;
	int match_count;

	s_obj(): pmdl(NULL), name(NULL), is_attitude_fixed(false), 
		roll(0.0), pitch(0.0), yaw(0.0), pos(0., 0., 0.),
		update(false), bb2d(0, 0, 0, 0), delta_f_rmax(DBL_MAX), delta_Tz_rmax(DBL_MAX),
		delta_Tx_rmax(-DBL_MAX), delta_Ty_rmax(-DBL_MAX)
	{
		tvec = Mat::zeros(3, 1, CV_64FC1);
		rvec = Mat::zeros(3, 1, CV_64FC1);
	};

	~s_obj()
	{
		delete[] name;
		name = NULL;
	}

	int calc_num_matched_points();
	double calc_ssd();

	int get_num_points(){
		return (int) pt2d.size();
	}

	void get_bb_pt2d(Rect & bb);

	// calculate projection
	void proj(Mat & camint, Mat & camdist, bool bjacobian = true, bool fix_aspect_ratio = true);

	void render(Mat & img);

	bool init(s_model * apmdl, long long at, const Mat & camint, const Mat & camdist,
		const double width, const double height);

	bool init(const s_obj & obj);

	void calc_part_deformation();

	bool load(FileNode & fnobj, vector<s_model*> & mdls);
	bool save(FileStorage & fs);

	void fixAttitude(bool val)
	{
		is_attitude_fixed = val;
	}

	void sample_tmpl(Mat & img, Size & sz); 
	void sample_pt_tmpl(int ipt, Mat & img, Size & sz);
	void free_tmpl(){
		ptx_tmpl.clear();
	}

	double delta_f_rmax, delta_Tz_rmax, delta_Tx_rmax, delta_Ty_rmax;
	double delta_Tz_rmax_trn, delta_Tx_rmax_trn, delta_Ty_rmax_trn;
	void analyze_error(double fx, double fy);
};


#endif

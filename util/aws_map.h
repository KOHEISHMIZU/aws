#ifndef _AWS_MAP_H_
#define _AWS_MAP_H_
// Copyright(c) 2017 Yohei Matsumoto, All right reserved. 

// aws_map.h is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// aws_map.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with aws_map.h.  If not, see <http://www.gnu.org/licenses/>. 
#define GR 1.61803398875 // golden ratio

namespace AWSMap2 {

  struct vec3{
    union{
      double x;
      double lat;
    };
    
    union{
      double y;
      double lon;
    };
    
    union {
      double z;
      double alt;
    };
    
  vec3() :x(0), y(0), z(0)
    {
    }
    
  vec3(const double _x, const double _y, const double _z) : x(_x), y(_y), z(_z)
    {
    }
    
    const vec3 & operator *= (const double s)
    {
      x *= s; y *= s; z *= s;
      return *this;
    }
    
    const vec3 & operator += (const vec3 & r)
    {
      x += r.x; y += r.y; z += r.z;
      return *this;
    }
  };
  
  inline bool operator == (const vec3 & l, const vec3 & r)
  {
    return l.x == r.x && l.y == r.y && l.z == r.z;
  }
  
  inline vec3 operator - (const vec3 & l, const vec3 & r)
  {
    return vec3(l.x - r.x, l.y - r.y, l.z - r.z);
  }
  
  inline vec3 operator + (const vec3 & l, const vec3 & r)
  {
    return vec3(l.x + r.x, l.y + r.y, l.z + r.z);
  }
  
  inline vec3 operator * (const vec3 & l, const double & r)
    {
      return vec3(l.x * r, l.y * r, l.z * r);
    }
  
  inline double dot(const vec3 & l, const vec3 & r)
  {
    return l.x * r.x + l.y * r.y + l.z * r.z;
  }
  
  struct vec2{
    union{
      double x;
      double lat;
    };
    
    union{
      double y;
      double lon;
    };

  vec2() :x(0.), y(0.)
    {
    }
    
  vec2(const double _x, const double _y) :x(_x), y(_y)
    {
    }
  };
  
  inline bool operator == (const vec2 & l, const vec2 & r)
  {
    return l.x == r.x && l.y == r.y;
  }
  
  
  inline double l2Norm2(const vec3 & pt0, const vec3 & pt1)
  {
    double tmp, result = 0.0;
    tmp = pt0.x - pt0.x;
    result = tmp * tmp;
    
    tmp = pt0.y - pt1.y;
    result += tmp * tmp;
    
    tmp = pt0.z - pt1.z;
    result += tmp * tmp;
    
    return result;
  }
  
  inline double l2Norm(const vec3 & pt0, const vec3 & pt1)
  {
    return sqrt(l2Norm2(pt0, pt1));
  }
  
  enum {
    MAX_PATH_LEN=1024
  };
  
  enum LayerType {
    lt_coast_line=0, lt_undef
  };
  extern const char * strLayerType[lt_undef];
  LayerType getLayerType(const char * str);
  class Node;
  class LayerData;
  
  class MapDataBase
  {
  private:
    static unsigned int maxSizeLayerData[lt_undef]; // maximum size of each LayerData instance
    static unsigned int maxNumNodes;			// maximum number of Node instances
    static unsigned int maxTotalSizeLayerData;	// maximum total size of LayerData instances
    static char * path;							// local storage path to save Node and LayerData. 
    
  public:
    static unsigned int getMaxSizeLayerData(const LayerType & layerType)
    {
      return maxSizeLayerData[layerType];
    }
    
    static void setMaxSizeLayerData(const LayerType & layerType, const unsigned int size)
    {
      maxSizeLayerData[layerType] = size;
    }
    
    static unsigned int getMaxNumNodes()
    {
      return maxNumNodes;
    }
    static void setMaxNumNodes(const unsigned int _maxNumNodes)
    {
      maxNumNodes = _maxNumNodes;
    }
    
    static unsigned int getMaxTotalSizeLayerData()
    {
      return maxTotalSizeLayerData;
    }
    
    static void setMaxTotalSizeLayerData(const unsigned int _maxTotalSizeLayerData)
    {
      maxTotalSizeLayerData = _maxTotalSizeLayerData;
    }
    
    static const char * getPath();
    static void setPath(const char * path);
    
  private:
    Node * pNodes[20];							// 20 triangles of the first icosahedron
    
  public:
    MapDataBase();
    virtual ~MapDataBase();

    bool init();
    
    // request layerData within the circle specified with (center, radius).
    void request(list<list<const LayerData*>> & layerDatum, const list<LayerType> & layerTypes,
		 const vec3 & center, const float radius, const float resolution = 0);
    
    // insert an instance of LayerData to the location.
    bool insert(const LayerData * layerData);
    
    // erase an instance of LayerData. The instance should be got via request method
    bool erase(const LayerData * layerData);
    
    // restruct MapDataBase on the memory, as the number of nodes and total size of layer data are to be under their limits.
    // (this method should be called periodically)
    void restruct();
    
    // save MapDataBase (only the parts updated)
    bool save();
  };
  
  class Node
  {
  private:
    static Node * head, * tail;
    static unsigned int numNodesAlive;
    static void insert(Node * pNode);
    static void pop(Node * pNode);
    static void accessed(Node * pNode);
  public:
    static void restruct();
    static Node * load(Node * pNodeUp, unsigned int idChild);
    
  private:
    Node * prev, * next;// link pointers for memory management
    
    bool bupdate;		// update flag. asserted when the layerDataList or downLink is updated
    unsigned char id;	// id of the node in the upper node. (0 to 3 for ordinal nodes. 0 to 19 for top level nodes.)
    Node * upLink;		// Up link. NULL for top 20 nodes
    bool bdownLink;		// false until the downLink is created.
    Node * downLink[4]; // Down link. 
    vec2 vtx_bih[3];	// bih coordinte of the node's triangle
    vec3 vtx_ecef[3];   // ecef coordinate of the node's triangle (calculated automatically in construction phase) 
    map<LayerType, LayerData*> layerDataList;
    
    // create downLink nodes, and assert bdownLink flag. 
    // the function is called only in addLayerData when the addition exceeds the 
    // size limit of the layer type. 
    bool createDownLink();
    
    // getPath(list<unsigned char>) helps getPath(char*, unsigned int) to generate path string to this node.
    void getPath(list<unsigned char> & path_id);
    
    // insertLayerData helps addLayerData. 
    void insertLayerData(const LayerType layerType, LayerData * pLayerData)
    {
      layerDataList.insert(pair<LayerType, LayerData*>(layerType, pLayerData));
    }
    
    // getLayerData returns layerData of layerType in this node 
    const LayerData * getLayerData(const LayerType layerType);
    
    // distributeLayerData helps addLayerData. 
    bool distributeLayerData(const LayerData & layerData);
    
    // relleaseLayerData releases all the layer data in the node.
    void releaseLayerData();
    
  public:
    Node();
    Node(const vec2 vtx_bih0, const vec2 vtx_bih1, const vec2 vtx_bih2);
    
    virtual ~Node();
    
    // setId(unsigned char) set node index in the upper layer.
    void setId(const unsigned char _id){
      id = _id;
    }
    
    // getPath(char*, unsigned int) returns the path string the length is less than the specified limit.
    void getPath(char * path, unsigned int maxlen);
    
    // save Node data and layer data recursively for all downlinks
    // this function is called only from MapDataBase::save()
    bool save();
    
    // collision(vec3) determines whether the specified point collides with the node.
    const bool collision(const vec3 & location);
    
    // collision(vec3, float) determines whether the specified circle collides with the node.
    const bool collision(const vec3 & center, const float radius);
    
    // getLayerData called from MapDataBase::request
    const void getLayerData(list<list<const LayerData*>> & layerData, 
			    const list<LayerType> & layerType, const vec3 & center,
			    const float radius, const float resolution = 0);
    
    // addLayerData adds the layer data given in the argument.
    // the function is invoked from LayerData::split, and the split is called from MapDataBase::insert
    // This function recursively call split() and itself so that the size of the layer data does not exceeds its limit.
    bool addLayerData(const LayerData & layerData, 
		      const size_t sz_node_data_lim = 0x4FFFFF /* 4MB */);
    
    // deleteLayerData deletes the layer data given in the argument.
    // The method delete the layer data in the node when the pointer is exactly the same as that given in the argument.
    // so the pointer given as the argument should be got via getLayerData
    bool deleteLayerData(const LayerData * layerData);	
  };
  
  class LayerData
  {
    // static section
  private:
    static LayerData * head, * tail;
    static unsigned int totalSize;
  protected:
    static void insert(LayerData * pLayerData);
    static void pop(LayerData * pLayerData);
  public:
    static void accessed(LayerData * pLayerData);
    static void restruct();
    static LayerData * create(const LayerType layerType); // factory function
    
  protected:
    LayerData * prev, * next;
    bool bupdate;
    bool bactive;
    
    Node * pNode;
    
    void genFileName(char * fname, size_t len_max)
    {
      char path[2048];
      pNode->getPath(path, 2048);
      snprintf(fname, len_max, "%s/%s.dat", path, strLayerType[getLayerType()]);
    }
    
  public:
  LayerData() : prev(NULL), next(NULL), pNode(NULL), bupdate(true), bactive(false) {};
    virtual ~LayerData() {};

    
    void setNode(Node * _pNode) { pNode = _pNode; };
    Node * getNode() const { return pNode; };
    
    void setActive(){
      bactive = true;
      LayerData::insert(this);
    }
    
    bool isActive(){
      return bactive;
    }
    
    // major interfaces 
    bool save();
    bool load();
    void release();
    bool reduce(const size_t sz_lim);
    bool merge(const LayerData & layerData);
    
    // interfaces to be implemented in sub-classes.
  protected:
    virtual bool _reduce(const size_t sz_lim) = 0;			// reduce the data structure to meet the size limit
    virtual bool _merge(const LayerData & layerData) = 0;	// merge given layerData to this layerData
    virtual void _release() = 0;							// release all internal data structure but does not mean the destruction of this object
    
  public:
    virtual const LayerType getLayerType() const = 0; // returns LayerType value.
    virtual bool save(ofstream & ofile) = 0;// save data to ofile stream.
    virtual bool load(ifstream & ifile) = 0;// load data from ifile stream.
    virtual bool split(list<Node*> & nodes) const = 0; // split the layer data into nodes given
    virtual LayerData * clone() const = 0;	// returns clone of the instance
    virtual size_t size() const = 0;		// returns size in memory 
    virtual float resolution() const = 0;	// returns minimum distance between objects in meter		
    virtual float radius() const = 0;		// returns radius of the object's distribution in meter
    virtual vec3 center() const = 0;	// returns center of the object's distribution
  };
  
  class CoastLine : public LayerData
  {
  protected:
    static const vector<vec3> null_vec_vec3;
    struct s_line {
      unsigned int id;
      vector<vec2> pts;
      vector<vec3> pts_ecef;
      
      size_t size() {
	return sizeof(unsigned int) + (sizeof(vec2) + sizeof(vec3)) * pts.size();
      }
    };
    size_t total_size;
    double dist_min;
    double pt_radius;
    vec3 pt_center;
    vector<s_line*> lines;
    void add(list<vec2> & line);
    int try_reduce(int nred);
    void update_properties();
  public:
    CoastLine();
    virtual ~CoastLine();
    
    const unsigned int getNumLines() const
    {
      return lines.size();
    }
    
    const vector<vec3> & getPointsECEF(unsigned int id) const
    {
      if (id >= lines.size())
	return null_vec_vec3;
      return lines[id]->pts_ecef;
    }
    
    bool loadJPJIS(const char * fname);
  protected:
    virtual bool _reduce(const size_t sz_lim);
    virtual bool _merge(const LayerData & layerData);
    virtual void _release();
  public:
    virtual const LayerType getLayerType() const { return lt_coast_line; };
    virtual bool save(ofstream & ofile);
    virtual bool load(ifstream & ifile);
    virtual bool split(list<Node*> & nodes) const;
    virtual LayerData * clone() const;
    virtual size_t size() const;
    virtual float resolution() const;
    virtual float radius() const; // returns radius of the object's distribution in meter
    virtual vec3 center() const; // returns center of the object's distribution
  };
};

class c_icosahedron
{
 private:
  unsigned int nv, nf, ne; // number of vertices, faces, edges
  
  Point3f *v; // vertex in ecef
  Point2f *q; // vertex in lat/lon
  unsigned int ** f;   // face (index)
  unsigned int ** e;   // edge (index)
  
 public:
  c_icosahedron();
  c_icosahedron(const c_icosahedron & icshdrn);
  ~c_icosahedron();
  Point3f get_mid_point(unsigned int ie) const
  {
    return (v[e[ie][0]] + v[e[ie][1]]) * 0.5;
  }
  
  unsigned int get_edge(unsigned int iv0, unsigned int iv1) const
  {
    if (iv0 > iv1){
      int t = iv0;
      iv0 = iv1;
      iv1 = t;
    }
    for (unsigned int ie = 0; ie < ne; ie++)
      {
	if (e[ie][0] == iv0 && e[ie][1] == iv1)
	  return ie;
      }
    
    return UINT_MAX;
  }
  
  const unsigned int get_nv() const
  {
    return nv;
  }
  
  const unsigned int get_nf() const
  {
    return nf;
  }
  
  const unsigned int get_ne() const
  {
    return ne;
  }
  
  Point3f * getv(){
    return v;
  }
  
  Point2f * getq(){
    return q;
  }
  
  unsigned int ** getf(){
    return f;
  }
};

#endif
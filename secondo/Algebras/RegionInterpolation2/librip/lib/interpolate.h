/* 
 1 interpolate.h is the main include file for the RegionInterpolation2Algebra.
 All classes and functions are declared here.

*/

#ifndef INTERPOLATE_HXX
#define INTERPOLATE_HXX

#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>

#include "config.h"
#include "librip.h"

#define DEBUG(l,m) do{if(l<=DEBUGLEVEL){std::cerr<<m<<std::endl;}} while (0)

#if defined(LUA5_1) || defined(LUA5_2)
#define USE_LUA
#define LUA_FUNCTION(FNAME) int luafunc_ ## FNAME(lua_State *L)
#define LUA_ADD_FUNCTION(FNAME) lua_pushcfunction(L, luafunc_ ## FNAME); \
                                lua_setglobal(L, #FNAME)
#endif


// Forward-declarations of the classes
class Poly;
class Pt;
class Seg;
class Face;
class MSeg;
class MSegs;
class MFace;
class MFaces;

class Interval { // a time interval
public:
    double start; // start point in ms since epoch
    double end;   // end point in ms since epoch
    bool lc;      // left-closed interval?
    bool rc;      // right-closed interval?
    
    Interval(double start, double end, bool lc, bool rc) : 
             start(start), end(end), lc(lc), rc(rc) {};
    Interval(std::string start, std::string end, bool lc, bool rc);
    Interval() : start(0), end(0), lc(true), rc(true) {};
    std::string startstr(); // start in YYYY-mm-dd-HH:MM:ss.SSS
    std::string endstr();   // end   in YYYY-mm-dd-HH:MM:ss.SSS
};


class Pt { // a point
public:
    // Fields
    double x, y;
    int valid;
    long double angle, dist;

    //Constructors
    Pt();
    Pt(double x, double y);
    
    // Operators
    bool operator<(const Pt& a) const;
    bool operator==(const Pt& a) const;
    Pt operator-(const Pt& a) const;
    Pt operator-() const; // Unary minus
    Pt operator+(const Pt& a) const;
    Pt operator/(double a) const;
    Pt operator*(const Pt& a) const;
    
    // Methods
    bool lessPolar(const Pt& a) const;  // comparison by polar angle/distance
    void calcPolar(const Pt& pt); // calculates polar coordinates with origin p
    double distance(Pt p); // calculates the distance to p
    std::string ToString() const;
    
    // Static functions
    static double sign(const Pt& a, const Pt& b, const Pt& c); // checks order
    static bool insideTriangle(const Pt& a, const Pt& b, const Pt& c,
            const Pt& x); // checks if x is inside the triangle (a b c)
};

class Seg { // a segment
public:
    // Fields
    Pt s, e;    // start- and endpoint
    int valid;

    // Constructors
    Seg();
    Seg(Pt s, Pt e);
    
    // Operators
    bool operator==(const Seg& a) const;
    bool operator<(const Seg& a) const;
    
    // Methods
    long double angle() const; // calculates angle with x-axis
    long double angle(Seg s) const; // calculates angle with a given segment
    void ChangeDir(); // changes the direction of the segment
    bool intersects(const Seg& seg) const; // checks for intersection with seg
    double sign (const Pt& p) const; // calculate the sign of segment and point
    std::string ToString() const;
};

class Face { // a face
private:
    int cur;

public:
    // Fields
    std::vector<Seg> v; 
                    // the segments of this face in ccw-order (start lower-left)
    std::vector<Face> holes; // the list holes of this face
    std::vector<Seg> convexhull; // the convex hull
    Face *parent; // the parent face of this face
    Pt peerPoint; // point on a peer-face to collapse to
    Seg hullSeg; // segment on hull of the parent face if this is a concavity
    bool ishole; // true if this face is a hole in another face
    int used; // used by matchFaces if this face was paired with another face
    int isdst; // is this face from the source- or destination-region
    std::pair<Pt, Pt> bbox; // bounding-box of this face

    // Constructors
    Face();
    Face(RList le, bool withHoles);   // Construct from RList
    Face(std::vector<Seg> v); // Construct from list of segments
    
    // Methods
    void AddSeg(Seg a); // Add a new segment
    void Close();       // Close the face
    Face ConvexHull();  // Calculate the convex hull
    Seg Begin(); // Traversal: Go to first segment
    Seg Next();  // Traversal: Advance one segment
    Seg Prev();  // Traversal: Go back one segment
    Seg Cur();   // Traversal: Get the current segment
    bool End();  // Traversal: Check if we are past the last segment
    void Sort(); // Sort the segments (start lower-left, counterclockwise)
    std::vector<Pt> getPoints(); 
                         // Get the list of corner points in correct order
    std::pair<Pt, Pt> GetBoundingBox(); // Get the bounding box of this face
    Pt GetMiddle(); // Get the center of the bounding box
    Pt GetCentroid(); // Calculate the centroid of this face
    MSegs collapse(bool close); // collapse the face
    MSegs collapse(bool close, Pt dst); // collapse the face to a given point
    MFace collapseWithHoles(bool close); // include the holes when collapsing
    Pt collapsePoint(); // Determine the best collapse point for this face
    MSegs GetMSegs(bool triangles); // convert this face to a static moving face
    double distance(Face r); // calculate the distance of the middle points
    double GetArea(); // calculate the area of this face
    Face ClipEar();  // Clip an ear for triangulation
    std::vector<MSegs> Evaporate(bool close); // calculate evaporation-msegs
    void AddHole (Face hole); // Add a new hole to this face
    bool isEmpty() const; // Check, if this is an empty face
    bool Check(); // Check, if this face is valid
    void IntegrateHoles(); // Integrate holes into the cycle for triangulation
    void Transform (Pt off, Pt scale); // Transform by offset and scale factor
    bool Merge (Face m); // Merge two adjacent faces into one
    bool inside (const Pt& p) const; // Point inside face test
    std::string ToString() const;
    Poly MakePoly (double offx, double offy, double scalex, double scaley,
                   bool withholes);
    Poly MakePoly (bool withholes);

    // Static functions
    static std::vector<Face> getFaces(RList le); // Get faces from RList
    static std::pair<Pt, Pt> GetBoundingBox(std::vector<Face> faces);
    static std::pair<Pt, Pt> GetBoundingBox(std::set<Face*> faces);
    static std::vector<Seg> sortSegs(std::vector<Seg> v); //Sort segs to a cycle
    static MFaces CreateMFaces(std::vector<Face> *faces);
};

class MSeg { // a moving segment
public:
    // Fields
    Pt is, ie, fs, fe; // Points of initial and final segment
    std::vector<Pt> ip, fp; // Intermediary points if MSeg was merged

    // Constructors
    MSeg();
    MSeg(Pt is, Pt ie, Pt fs, Pt fe); // Construct from segments' endpoints
    
    // Operators
    bool operator==(const MSeg& a) const;
    bool operator<(const MSeg& a) const;
    
    // Methods
    bool intersects(const MSeg& a, bool checkSegs) const;
    bool Merge(const MSeg& a); // Merge two MSegs if collinear
    bool Split(MSeg& n, MSeg& m1, MSeg& m2); // Split MSeg on "n"
    void ChangeDirection(); // Change the orientation
    MSeg divide(double start, double end); // restrict interval
    long double angle (MSeg ms); // Angle to another MSeg
    std::string ToString() const;
};

class MSegs { // a set of moving segments
public:
    // Fields
    std::vector<MSeg> msegs; // the msegs which make up a cycle
    Face sreg, dreg; // the faces from which this msegs were calculated
    int iscollapsed; // >0 if the face collapses or expands
    int isevaporating; // >0 if the face evaporates
    std::pair<Pt,Pt> bbox; // corner-points of the bounding box

    // Constructor
    MSegs();
    
    // Methods
    void AddMSeg(MSeg m); // Add a new MSeg to the cycle
    bool MergeConcavity(MSegs c); // Merge a concavity cycle if possible
    void updateBBox(MSeg& seg); // Update the bounding box after adding seg
    std::pair<Pt, Pt> calculateBBox(); // Calculate the bounding box
    Face CreateBorderFace(bool initial); // Create static mface from border
    int findNext(int index, bool check); // get successor of segment at index
    int findNext(MSeg cur, int start, bool check); // get successor of cur
    int findNexta(MSeg cur, int start); // get successor of cur with small angle
    MSegs divide(double start, double end); // restrict the interval
    std::pair<MSegs, MSegs> kill(); // create pair of expanding/collapsing faces
    bool intersects(const MSegs& a, bool matchIdent, bool matchSegs) const;
    std::string ToString() const;
    
};

class MFace { // a complete cycle of moving segments aka moving face
public:
    // Fields
    MSegs face; // the moving segments which make up this face
    bool needStartRegion, needEndRegion; // this face needs a start/end-region
    std::vector<MSegs> holes; // list of holes
    std::vector<MFace> cvs; // list of pending concavities
    std::pair<Pt, Pt> bbox; // the bounding box of this mface

    // Constructors
    MFace();
    MFace(MSegs face); // Construct from an MSegs-object
    
    // Methods
    bool Check();                // Check if this MFace is a valid cycle
    bool SortCycle();            // Sort this cycle
    std::vector<MFace> SplitCycle(); // Sort and split this cycle if possible
    void EliminateSpikes();      // Eliminate empty spikes after merge
    void AddConcavity(MFace c);  // Add a concavity to this cycle
    std::vector<MFace> MergeConcavities();  //Merge previously added concavities
    RList ToListExpr();       // Create a list expression
    MFace divide(double start, double end); // restrict the interval
    Face CreateBorderFace(bool src); // get face from border of the interval
    void PrintMRegionListExpr(); // Output list expression for debugging
    bool isEmpty(); // Check, if this is an empty mface
    std::string ToString();
};

class MFaces { // a set of moving faces
public:
    // Fields
    std::vector<MFace> faces; // List of mface-objects
    std::vector<Face> *sregs, *dregs; //lists of faces from which it was created
    bool needSEvap, needDEvap; // evaporation- or condensation-phase is needed

    // Constructor
    MFaces();
    MFaces(MFace face);
    
    // Methods
    void AddMFace(MFace face); // Add a new mface to this mfaces-object
    RList ToListExpr(Interval iv, double start, double end);
    RList ToMListExpr(Interval iv); // create RList-Expressions
    MFaces divide(double start, double end); // restrict the interval
    std::vector<Face> CreateBorderFaces(bool src); // create faces from border
    MFaces CreateBorderMFaces(bool src); // create static mfaces from border
    std::string ToString();
    
    // Static functions
    static RList fallback(std::vector<Face> *s, std::vector<Face> *d, 
                          Interval iv);
};

class RotatingPlane { // the rotating plane algorithm
public:
    // Fields
    MFace mface; // mface created by rotating plane algorithm
    std::vector<Face> scvs, dcvs; // source- and destination-concavity

    // Constructor (this constructor already does all the work)
    RotatingPlane(Face *src, Face *dst, int depth, bool evap);
};



// Function prototypes

MFaces interpolate(std::vector<Face> *sregs, std::vector<Face> *dregs, 
                   int depth,
        bool evap, std::string args);

std::vector<std::pair<Face *, Face *> > matchFaces(
        std::vector<Face> *src, std::vector<Face> *dst, int depth,
        std::string args);

typedef std::vector<std::pair<Face *, Face *> > 
                      (*matchFaces_t)(std::vector<Face> *src,
        std::vector<Face> *dst, int depth, std::string args);

// Prototypes of the matchFaces-strategies
std::vector<std::pair<Face *, Face *> > matchFacesSimple(std::vector<Face> *src,
        std::vector<Face> *dst, int depth, std::string args);
std::vector<std::pair<Face *, Face *> > matchFacesNull(std::vector<Face> *src,
        std::vector<Face> *dst, int depth, std::string args);
std::vector<std::pair<Face *, Face *> > 
                       matchFacesLowerLeft(std::vector<Face> *src,
        std::vector<Face> *dst, int depth, std::string args);
std::vector<std::pair<Face *, Face *> > 
                 matchFacesDistance(std::vector<Face> *src,
        std::vector<Face> *dst, int depth, std::string args);
std::vector<std::pair<Face *, Face *> > matchFacesLua(std::vector<Face> *src,
        std::vector<Face> *dst, int depth, std::string args);

#include "poly.h"

#endif /* INTERPOLATE_HXX */

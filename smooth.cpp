// Written on Mac OS X 10.6

/*
 * by Ryan Barril 
 * CS 418 - Interactive Graphics
 * MP4 - "Smooth I"
 * Created 12/4/2012
 */

#include <vector>
#include <map>
#include <GLUT/glut.h>
#include "math.h"
using namespace std;

#ifndef max
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

class Mesh;
class Face;
class Edge;
class Vertex;

int FPS = 60;		// frames per second
float xLoc = 0;		// horizontal starting position
Mesh * mesh;
int numSubDivides = 0;

/* Camera interpolation variables */
float p[][3] = {
{0.0, 0.0, 3.0},
{-15.0, 20.0, -20.0},
{15.0, -10.0, 10.0},
{0.0, 0.0, 3.0},
};

float cam_x = 0.f, cam_y = 0.f, cam_z = 3.f; // initial camera position
float t = 0.0;

class Mesh {
public:
	vector<Face *> fl; // facelist
	vector<Vertex *> vl; // vertexlist
};

class Vertex {
public:
	float x, y, z;
	int valence;
	vector<Face *> fl;
	vector<Edge *> el;
	Vertex * newV;
	
	Vertex(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
		valence = 0;
		newV = NULL;
	}
	
	bool equals(Vertex * u) {
		return (x == u->x && y == u->y && z == u->z);
	}
};

class Edge {
public:
	Vertex * v1, * v2;
	Vertex * center;
	Vertex * newE;
	float ID;
	
	Edge(Vertex * v1, Vertex * v2) {
		this->v1 = v1;
		this->v2 = v2;
		center = new Vertex((v1->x + v2->x) / 2,
							(v1->y + v2->y) / 2,
							(v1->z + v2->z) / 2);
		newE = NULL;
		if (v1->x < v2->x || v1->y < v2->y || v1->z < v2->z) {
			ID = 1 * v1->x + 10 * v1->y + 100 * v1->z + 1 * v2->x + 10 * v2->y + 100 * v2->z;
		} else {
			ID = 1 * v2->x + 10 * v2->y + 100 * v2->z + 1 * v1->x + 10 * v1->y + 100 * v1->z;
		}
	}
};

class Face {
public:
	Vertex * v[4];
	Vertex * centroid;
	Edge * e[4];
	
	Face(Vertex * v0, Vertex * v1, Vertex * v2, Vertex * v3) {
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
		v[3] = v3;
		centroid = new Vertex((v0->x + v1->x + v2->x + v3->x) / 4.0,
							  (v0->y + v1->y + v2->y + v3->y) / 4.0,
							  (v0->z + v1->z + v2->z + v3->z) / 4.0);
		v0->fl.push_back(this);
		v1->fl.push_back(this);
		v2->fl.push_back(this);
		v3->fl.push_back(this);
	}
	
	void draw() {
		glVertex3f(v[0]->x, v[0]->y, v[0]->z);
		glVertex3f(v[1]->x, v[1]->y, v[1]->z);
		glVertex3f(v[2]->x, v[2]->y, v[2]->z);
		glVertex3f(v[3]->x, v[3]->y, v[3]->z);
	}
	
	void drawOutlines() {
		glLineWidth(4.0);
		glColor3f(0.0, 0.0, 0.0);
		glVertex3f(v[0]->x, v[0]->y, v[0]->z);
		glVertex3f(v[1]->x, v[1]->y, v[1]->z);
		glVertex3f(v[2]->x, v[2]->y, v[2]->z);
		glVertex3f(v[3]->x, v[3]->y, v[3]->z);
	}
};

void drawFaces() {
	for (int i = 0; i < mesh->fl.size(); i++) {
		//glColor3f(1.0, 1.0, 1.0);
		glColor3f(.76, .296, .09);
		glBegin(GL_QUADS);	
		mesh->fl[i]->draw();
		glEnd();
		
		glBegin(GL_LINE_LOOP);
		mesh->fl[i]->drawOutlines();
		glEnd();
	}		
}

// Cubic Bezier Curve
float interpolate(int c) {
	return pow(1-t, 3) * p[0][c] + 3 * t * (1-t)*(1-t) * p[1][c] 
				+ 3 * (1 - t) * t*t * p[2][c] + t*t*t * p[3][c];
}

void moveCamera() {
	t += 0.002;
	double intpart;
	t = modf(t, &intpart);
	cam_x = interpolate(0);
	cam_y = interpolate(1);
	cam_z = interpolate(2);
}

struct eComp : public binary_function< Edge *, Edge *, bool >
{
    bool operator()(Edge *e1, Edge *e2) const {
		return e1->ID < e2->ID;
	}
};

void subdivide() {
	Mesh * newMesh = new Mesh;
	
	int numFaces = mesh->fl.size();
	
	map<Edge *, vector<Face *>, eComp> el; // edge to faces map
	
	// iterate over all faces one time to record edges and find valences
	for (int i = 0; i < numFaces; i++) {
		Face * f = mesh->fl[i];
		
		// record this face's edges
		for (int i = 0; i < 4; i++) {
			Vertex * vi = f->v[i];
			Vertex * vj = f->v[(i + 1) % 4];
			
			Edge * e = new Edge(vi, vj);
			
			vector<Face *> faceVec;
			map<Edge*, vector<Face *> >::iterator iter;
			if ((iter = el.find(e)) == el.end()) { // edge not yet recorded
				vi->el.push_back(e);
				vj->el.push_back(e);
				vi->valence++;
				vj->valence++;
				f->e[i] = e;
				faceVec.push_back(f);
				el.insert(make_pair(e, faceVec));
			}
			else { // need to add second face
				f->e[i] = (*iter).first;
				vector<Face *> faceVec = (*iter).second;
				faceVec.push_back(f);
				el[e] = faceVec;
			}
		}
	}
	
	// iterate over all faces second time to compute new faces
	for (int i = 0; i < numFaces; i++) {
		Face * f = mesh->fl[i];
		
		vector<Vertex *> edgepoints;
		vector<Vertex *> vertexpoints;
		
		// compute edgepoints
		for (int j = 0; j < 4; j++) {	
			Edge * e = f->e[j];
			
			if (e->newE != NULL) {
				edgepoints.push_back(e->newE);
				continue;
			}
			vector<Face *> faceVec;
			map<Edge*, vector<Face *> >::iterator iter;	
			iter = el.find(e);
			faceVec = (*iter).second;
			
			float xSum = e->v1->x + e->v2->x; float xPts = 2;
			float ySum = e->v1->y + e->v2->y; float yPts = 2;
			float zSum = e->v1->z + e->v2->z; float zPts = 2;
			for (int k = 0; k < faceVec.size(); k++) {
				Face * adjFace = faceVec[k];
				Vertex * cent = adjFace->centroid;
				xSum += cent->x; xPts++;
				ySum += cent->y; yPts++;
				zSum += cent->z; zPts++;
			}
			Vertex * edgepoint = new Vertex(xSum / xPts, ySum / yPts, zSum / zPts);		
			e->newE = edgepoint;
			edgepoints.push_back(edgepoint);
		}
		
		// compute vertexpoints
		for (int j = 0; j < 4; j++) {
			Vertex * v = f->v[j];
			
			if (v->newV != NULL) {
				vertexpoints.push_back(v->newV);
				continue;
			}
			
			float qx = 0.0; float qxpts = 0.0;
			float qy = 0.0; float qypts = 0.0;
			float qz = 0.0; float qzpts = 0.0;
			for (int k = 0; k < v->fl.size(); k++) {
				Face * adjFace = v->fl[k];
				qx += adjFace->centroid->x; qxpts++;
				qy += adjFace->centroid->y; qypts++;
				qz += adjFace->centroid->z; qzpts++;
			}
			Vertex * q = new Vertex(qx / qxpts, qy / qypts, qz / qzpts);
			
			float rx = 0.0; float rxpts = 0.0;
			float ry = 0.0; float rypts = 0.0;
			float rz = 0.0; float rzpts = 0.0;
			for (int m = 0; m < v->el.size(); m++) {
				Edge * adjEdge = v->el[m];
				rx += adjEdge->center->x; rxpts++;
				ry += adjEdge->center->y; rypts++;
				rz += adjEdge->center->z; rzpts++;
			}
			Vertex * r = new Vertex(rx / rxpts, ry / rypts, rz / rzpts);
			
			float n = v->valence;
			Vertex * vertexpoint = new Vertex((q->x / n + 2 * r->x / n + (n - 3) * v->x / n),
											  (q->y / n + 2 * r->y / n + (n - 3) * v->y / n),
											  (q->z / n + 2 * r->z / n + (n - 3) * v->z / n));
			
			v->newV = vertexpoint;
			vertexpoints.push_back(vertexpoint);
		}
		
		Face * f1 = new Face(vertexpoints[0], edgepoints[0], f->centroid, edgepoints[3]);		
		Face * f2 = new Face(edgepoints[0], vertexpoints[1], edgepoints[1], f->centroid);		
		Face * f3 = new Face(f->centroid, edgepoints[1], vertexpoints[2], edgepoints[2]);		
		Face * f4 = new Face(edgepoints[3], f->centroid, edgepoints[2], vertexpoints[3]);
		
		newMesh->fl.push_back(f1);
		newMesh->fl.push_back(f2);
		newMesh->fl.push_back(f3);
		newMesh->fl.push_back(f4);
	}
	
	mesh = newMesh;
}

float fRotateAngle = 0.f;

void display(void) {		
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);	// reset OpenGL transformation matrix
	glLoadIdentity();			// reset transformation matrix to identity
	
	// set view
	// eye is initially at : (0,0,3)
	// look at center is at : (0,0,0)
	// up direction is +y axis
	gluLookAt(cam_x, cam_y, cam_z,
			  0.f,0.f,0.f,
			  0.f,1.f,0.f);
	
	drawFaces();
	moveCamera();
	
	glFlush();
	glutSwapBuffers();	// swap front/back framebuffer to avoid flickering 
}

void reshape (int w, int h) {	
	// reset OpenGL projection matrix
	float fAspect = ((float) w) / h; 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70.f, fAspect, 0.001f, 15.f);
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 27) 
		exit(0); // ESC hit, so quit
	if (key == 's' && numSubDivides < 3) {
		subdivide();
		numSubDivides++;
	}
}

void timer(int v) {
	glutPostRedisplay();				// trigger display function again
	glutTimerFunc(1000/FPS,timer,v);	// restart timer again
}

void setupVertices(Mesh * m) {
	m->vl.push_back(new Vertex(-0.6, -1.0, 0.0)); //0
	m->vl.push_back(new Vertex(-0.6, -0.6, 0.0)); //1
	m->vl.push_back(new Vertex(-0.6, 0.6, 0.0)); //2
	m->vl.push_back(new Vertex(-0.6, 1.0, 0.0)); //3
	m->vl.push_back(new Vertex(-0.2, -1.0, 0.0)); //4
	m->vl.push_back(new Vertex(-0.2, -0.6, 0.0)); //5
	m->vl.push_back(new Vertex(-0.2, 0.6, 0.0)); //6
	m->vl.push_back(new Vertex(-0.2, 1.0, 0.0)); //7
	m->vl.push_back(new Vertex(0.2, -1.0, 0.0)); //8
	m->vl.push_back(new Vertex(0.2, -0.6, 0.0)); //9
	m->vl.push_back(new Vertex(0.2, 0.6, 0.0)); //10
	m->vl.push_back(new Vertex(0.2, 1.0, 0.0)); //11
	m->vl.push_back(new Vertex(0.6, -1.0, 0.0)); //12
	m->vl.push_back(new Vertex(0.6, -0.6, 0.0)); //13
	m->vl.push_back(new Vertex(0.6, 0.6, 0.0)); //14
	m->vl.push_back(new Vertex(0.6, 1.0, 0.0)); //15
	
	m->vl.push_back(new Vertex(-0.6, -1.0, 0.75)); //16
	m->vl.push_back(new Vertex(-0.6, -0.6, 0.75)); //17
	m->vl.push_back(new Vertex(-0.6, 0.6, 0.75)); //18
	m->vl.push_back(new Vertex(-0.6, 1.0, 0.75)); //19
	m->vl.push_back(new Vertex(-0.2, -1.0, 0.75)); //20
	m->vl.push_back(new Vertex(-0.2, -0.6, 0.75)); //21
	m->vl.push_back(new Vertex(-0.2, 0.6, 0.75)); //22
	m->vl.push_back(new Vertex(-0.2, 1.0, 0.75)); //23
	m->vl.push_back(new Vertex(0.2, -1.0, 0.75)); //24
	m->vl.push_back(new Vertex(0.2, -0.6, 0.75)); //25
	m->vl.push_back(new Vertex(0.2, 0.6, 0.75)); //26
	m->vl.push_back(new Vertex(0.2, 1.0, 0.75)); //27
	m->vl.push_back(new Vertex(0.6, -1.0, 0.75)); //28
	m->vl.push_back(new Vertex(0.6, -0.6, 0.75)); //29
	m->vl.push_back(new Vertex(0.6, 0.6, 0.75)); //30
	m->vl.push_back(new Vertex(0.6, 1.0, 0.75)); //31
}

void setupFaces(Mesh * m) {
	/* Cube
	Vertex * v1 = new Vertex(0.0, 1.0, 0.0);
	Vertex * v2 = new Vertex(0.0, 0.0, 0.0);
	Vertex * v3 = new Vertex(1.0, 0.0, 0.0);
	Vertex * v4 = new Vertex(1.0, 1.0, 0.0);
	Vertex * v5 = new Vertex(0.0, 1.0, 1.0);
	Vertex * v6 = new Vertex(0.0, 0.0, 1.0);
	Vertex * v7 = new Vertex(1.0, 0.0, 1.0);
	Vertex * v8 = new Vertex(1.0, 1.0, 1.0);
	
	m->fl.push_back(new Face(v1, v2, v3, v4)); // front
	m->fl.push_back(new Face(v5, v1, v4, v8)); // top
	m->fl.push_back(new Face(v5, v6, v2, v1)); // left
	m->fl.push_back(new Face(v4, v3, v7, v8)); // right
	m->fl.push_back(new Face(v5, v6, v7, v8)); // back
	m->fl.push_back(new Face(v6, v2, v3, v7)); // bottom
	*/
	
	m->fl.push_back(new Face(m->vl[7], m->vl[6], m->vl[10], m->vl[11]));
	m->fl.push_back(new Face(m->vl[6], m->vl[5], m->vl[9], m->vl[10]));
	m->fl.push_back(new Face(m->vl[5], m->vl[4], m->vl[8], m->vl[9]));
	m->fl.push_back(new Face(m->vl[3], m->vl[2], m->vl[6], m->vl[7]));
	m->fl.push_back(new Face(m->vl[1], m->vl[0], m->vl[4], m->vl[5]));
	m->fl.push_back(new Face(m->vl[9], m->vl[8], m->vl[12], m->vl[13]));
	m->fl.push_back(new Face(m->vl[11], m->vl[10], m->vl[14], m->vl[15]));
	m->fl.push_back(new Face(m->vl[22], m->vl[21], m->vl[25], m->vl[26]));
	m->fl.push_back(new Face(m->vl[21], m->vl[20], m->vl[24], m->vl[25]));
	m->fl.push_back(new Face(m->vl[19], m->vl[18], m->vl[22], m->vl[23]));
	m->fl.push_back(new Face(m->vl[17], m->vl[16], m->vl[20], m->vl[21]));
	m->fl.push_back(new Face(m->vl[25], m->vl[24], m->vl[28], m->vl[29]));
	m->fl.push_back(new Face(m->vl[27], m->vl[26], m->vl[30], m->vl[31]));
	m->fl.push_back(new Face(m->vl[23], m->vl[22], m->vl[26], m->vl[27]));

	m->fl.push_back(new Face(m->vl[3], m->vl[2], m->vl[18], m->vl[19]));
	m->fl.push_back(new Face(m->vl[1], m->vl[0], m->vl[16], m->vl[17]));
	m->fl.push_back(new Face(m->vl[12], m->vl[13], m->vl[29], m->vl[28]));
	m->fl.push_back(new Face(m->vl[14], m->vl[15], m->vl[31], m->vl[30]));
	m->fl.push_back(new Face(m->vl[2], m->vl[6], m->vl[22], m->vl[18]));
	m->fl.push_back(new Face(m->vl[1], m->vl[5], m->vl[21], m->vl[17]));
	m->fl.push_back(new Face(m->vl[9], m->vl[13], m->vl[29], m->vl[25]));
	m->fl.push_back(new Face(m->vl[10], m->vl[14], m->vl[30], m->vl[26]));
	m->fl.push_back(new Face(m->vl[3], m->vl[15], m->vl[31], m->vl[19]));
	m->fl.push_back(new Face(m->vl[0], m->vl[12], m->vl[28], m->vl[16]));
	m->fl.push_back(new Face(m->vl[6], m->vl[5], m->vl[21], m->vl[22]));
	m->fl.push_back(new Face(m->vl[10], m->vl[9], m->vl[25], m->vl[26]));

}

int main(int argc, char** argv) {
	
	mesh = new Mesh;
	setupVertices(mesh);
	setupFaces(mesh);
	
	glutInit(&argc, (char**)argv);
	
	// set up for double-buffering & RGB color buffer & depth test
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	
	// set up window
	glutInitWindowSize (1000, 500); 
	glutInitWindowPosition (100, 100);
	glutCreateWindow ((const char*) "Smooth I");
	
	glClearColor(1, 1, 1, 1.0); // set background color
	
	// set up the call-back functions 
	glutDisplayFunc(display);  // called when drawing 
	glutReshapeFunc(reshape);  // called when change window size
	glutKeyboardFunc(keyboard); // called when received keyboard interaction
	glutTimerFunc(100, timer, FPS); // a periodic timer. Usually used for updating animation
	
	glutMainLoop(); // start the main callback loop
	
	return 0;
}
#pragma once

#include "ofMain.h"
#include "ofxAssimpModelLoader.h"
#include "box.h"
#include "ray.h"
#include "Octree.h"
#include "ofxGui.h"
#include "ParticleEmitter.h"

// Ship Class
// Consolidates the fields and methods relevant to the
// Ship Model. Used for updating positioning and physics
// related movement of the Ship Model.
// --Jared Bechthold
//----------------------------------------------------
class Ship {
public:
	// Ship Constructor
	Ship(string src);

	// Fields
	ofxAssimpModelLoader shipModel;		// Holds the ship's model
	Box shipBBox;						// Holds bounding box of ship
	glm::vec3 position;					// Holds position of ship
	glm::vec3 velocity;					// Holds ship's velocity
	glm::vec3 acceleration;				// Holds ship's acceleration
	float rotation = 0.0;				// Holds rotation of ship
	float turnVelocity;					// Holds turn velocity of ship
	float turnAcceleration;				// Holds turn acceleration of ship
	bool shipSelected;					// Whether or not the ship is selected
	bool shipLoaded = false;			// Whether or not the ship is loaded
	float thrust;						// Holds the thrust force of ship
	ofVec3f appliedThrust;				// Thurst force in 3D 
	ofVec3f gravity;					// Gravity force in 3D
	ofVec3f impulseForce;				// Impulse force in 3D
	ofVec3f forces = ofVec3f(0, 0, 0);	// Combination of all forces
	float damping;						// Decreases force values
	bool landed = false;				// Whether or not the ship has landed
	float fuel;

	// Functions
	void integrate();						// Movement of ship in direction
	void integrateTurn();					// Turning of ship
	void setPosition(glm::vec3 newPos);		// Set position of the ship
	void setRotation();						// Set rotation of the ship
	bool getShipSelected();					// Returns whether or not the ship is selected
	bool getShipLoaded();					// Returns whether or not the ship is loaded
	Box getLanderBounds();					// Gets boundaries of lander bounding box
	ofxAssimpModelLoader getShipModel();	// Returns ship's model
	glm::vec3 getPosition();				// Returns ship's position
	void addForces();						// adds up all forces
	void updateBoundingBox();
};

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void gotMessage(ofMessage msg);
	void drawAxis(ofVec3f);
	void initLightingAndMaterials();
	void savePicture();
	void toggleWireframeMode();
	void togglePointsDisplay();
	void toggleSelectTerrain();
	void setCameraTarget();
	void drawBox(const Box &box);
	void loadVbo();
	Box meshBounds(const ofMesh &);
	bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point);
	bool raySelectWithOctree(ofVec3f &pointRet);
	bool raySelectLine(ofVec3f & pointRet);
	glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 p, glm::vec3 n);


	ofEasyCam cam;
	ofCamera top;
	ofCamera follow;
	ofCamera front;
	ofCamera ground;
	ofCamera *theCam;
	ofxAssimpModelLoader terrain;
	ofLight light;
	Box boundingBox;
	Ship *lander;
	bool bAltKeyDown;
	bool bCtrlKeyDown;
	bool bWireframe;
	bool bDisplayPoints;
	bool bPointSelected;
	bool bHide;
	bool pointSelected = false;
	bool bDisplayLeafNodes = false;
	bool bDisplayOctree = false;
	bool bDisplayBBoxes = false;
	bool bTerrainSelected;
	bool showNearest;
	bool inBounds;
	bool hideLights = true;
	ofVec3f selectedPoint;
	ofVec3f intersectPoint;
	glm::vec3 mouseDownPos, mouseLastPos;
	const float selectionRange = 4.0;

	// Octree Setup
	vector<Box> colBoxList;
	Octree octree;
	TreeNode selectedNode;
	bool bInDrag = false;
	ofxIntSlider numLevels;
	ofxPanel gui;
	vector<Box> bboxList;

	// Particle Emitter Fields
	ParticleEmitter emitter;
	ParticleEmitter explosion;

	// Apply Impulse for hitting ground
	void checkCollisions();

	// Creates a keymap to determine if specific keys are pushed/released
	map<int, bool> keymap;

	// Valid landing area bounding box
	Box validLandingArea;

	// Tracks player altitude
	float altitude;

	// Holds sound played when sprites collide and are removed
	ofSoundPlayer exhaust;
	ofSoundPlayer boom;

	// Holds lights
	ofLight keyLight, fillLight, rimLight;
	ofxVec3Slider keyLightPos, fillLightPos, rimLightPos;

	// Holds background image
	ofImage background;

	// Sets font for displaying text
	ofTrueTypeFont text;

	// Sets gameplay boolean variables
	bool gameOver;
	bool standBy;
	bool exploded;

	// textures
	//
	ofTexture particleTex;
	// shaders
	//
	ofVbo vbo;
	ofShader shader;
};
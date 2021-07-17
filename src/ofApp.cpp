#include "ofApp.h"
#include "Util.h"
#include <glm/gtx/intersect.hpp>

/*
3DLandingGame by Jared Bechthold
The object of the game is to land a Ship model in a given boundary
while also maintaining small impulse force from the landing to avoid
damaging the ship.
*/


//--------------------------------------------------------------
// Sets up the scene
// Instantiates camera objects, loads the models, loads sounds,
// sets initial fields of lander, sets initial fields of emitters,
// loads font, and sets initial fields of lights
//
void ofApp::setup() {
	// texture loading
	//
	ofDisableArbTex();	// disable rectangular textures

	// load textures
	//
	if (!ofLoadImage(particleTex, "images/dot.png")) {
		cout << "Particle Texture File: images/dot.png not found" << endl;
		ofExit();
	}

	// load the shader
	//
#ifdef TARGET_OPENGLES
	shader.load("shaders_gles/shader");
#else
	shader.load("shaders/shader");
#endif

	// Sets up gui sliders to control octree levels displayed
	// and to control positioning of the lights
	//
	gui.setup();
	gui.add(numLevels.setup("Number of Octree Levels", 1, 1, 10));
	gui.add(keyLightPos.setup("Key Light Position", ofVec3f(40, 15, 20), ofVec3f(-500, -500, -500), ofVec3f(500, 500, 500)));
	gui.add(fillLightPos.setup("Fill Light Position", ofVec3f(-85, 25, 80), ofVec3f(-500, -500, -500), ofVec3f(500, 500, 500)));
	gui.add(rimLightPos.setup("Rim Light Position", ofVec3f(0, 10, -60), ofVec3f(-500, -500, -500), ofVec3f(500, 500, 500)));
	bHide = true;

	// Loads sound effects used for exhaust from ship
	// and ship explosion
	boom.load("sounds/boom.mp3");
	exhaust.load("sounds/exhaust.mp3");

	// Sets boolean values for terrain and display features
	bWireframe = false;
	bDisplayPoints = false;
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bTerrainSelected = true;

	// Sets the default field values for the easyCam instance
	// which is the default camera displayed when the program begins
	//
	cam.setPosition(ofVec3f(200, 100, 200));
	cam.lookAt(ofVec3f(0, 0, 0));
	cam.setDistance(100);
	cam.setNearClip(.1);
	cam.setFov(65.5);

	ofSetVerticalSync(true);
	cam.disableMouseInput();
	ofEnableSmoothing();
	ofEnableDepthTest();
	ofEnableLighting();

	// Sets the current camera
	// camera pointer begins with easyCam
	//
	theCam = &cam;

	// Sets up the 3 point Lighting System
	keyLight.setup();
	keyLight.enable();
	keyLight.setAreaLight(1, 1);
	keyLight.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	keyLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	keyLight.setSpecularColor(ofFloatColor(1, 1, 1));
	keyLight.rotate(45, ofVec3f(0, 1, 0));
	keyLight.rotate(-45, ofVec3f(1, 0, 0));
	keyLight.setPosition(keyLightPos);

	fillLight.setup();
	fillLight.enable();
	fillLight.setSpotlight();
	fillLight.setScale(.05);
	fillLight.setSpotlightCutOff(15);
	fillLight.setAttenuation(2, .001, .001);
	fillLight.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	fillLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	fillLight.setSpecularColor(ofFloatColor(1, 1, 1));
	fillLight.rotate(-10, ofVec3f(1, 0, 0));
	fillLight.rotate(-45, ofVec3f(0, 1, 0));
	fillLight.setPosition(fillLightPos);

	rimLight.setup();
	rimLight.enable();
	rimLight.setSpotlight();
	rimLight.setScale(.05);
	rimLight.setSpotlightCutOff(30);
	rimLight.setAttenuation(.2, .001, .001);
	rimLight.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	rimLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	rimLight.setSpecularColor(ofFloatColor(1, 1, 1));
	rimLight.rotate(180, ofVec3f(0, 1, 0));
	rimLight.setPosition(rimLightPos);

	// Setup rudimentary lighting 
	initLightingAndMaterials();

	// Loads the terrain
	terrain.loadModel("geo/moon-houdini.obj");
	terrain.setScaleNormalization(false);
	boundingBox = meshBounds(terrain.getMesh(0));

	// Create Octree
	octree.create(terrain.getMesh(0), 20);

	// Sets the initial fields of the Ship instance lander
	//
	lander = new Ship("geo/lander.obj");			// Loads lander model
	lander->thrust = 25.0;							// Sets thurst of lander
	lander->gravity = ofVec3f(0, -8.0, 0);			// Sets gravity force applied to lander
	lander->damping = 0.99;							// Sets damping of lander
	lander->acceleration = glm::vec3(0, 0, 0);		// Sets initial acceleration of lander
	lander->turnVelocity = 0;						// Sets the initial turn acceleration of lander
	lander->velocity = glm::vec3(0, 0, 0);			// Sets initial velocity of lander
	lander->turnVelocity = 0;						// Sets initial turn velocity of lander
	lander->rotation = 0;							// Sets initial rotation angle of lander
	lander->setPosition(ofVec3f(-50, 30, -50));		// Sets initial position of lander
	lander->fuel = 200;								// Sets fuel of lander
	lander->shipSelected = false;					// Lander begins not being selected

	// Fields for follow camera
	// Shows perspective from side view of lander
	follow.setNearClip(.1);
	follow.setFov(65.5);

	// Fields for top view camera
	// Shows perspective from looking right below lander
	top.setPosition(lander->getPosition());
	top.lookAt(ofVec3f(lander->getPosition().x, 1, lander->getPosition().z));
	top.setNearClip(.1);
	top.setFov(65.5);

	// Fields for top view camera
	// Shows perspective following lander from position
	// on ground near landing zone
	ground.setPosition(0, 5, 50);
	ground.setNearClip(.1);
	ground.setFov(65.5);

	// Fields for top view camera
	// Shows perspective from in-front of lander
	front.lookAt(glm::vec3(0, 0, -5));
	front.setNearClip(.1);
	front.setFov(65.5);

	// Sets up Emitter for lander exhaust effect
	emitter.setRate(2.5);
	emitter.setLifespan(0.25);
	emitter.setParticleRadius(0.05);
	emitter.setEmitterType(DiscEmitter);
	emitter.setGroupSize(250);

	// Sets up Explosion Emitter
	explosion.setRate(2.5);
	explosion.setLifespan(1.0);
	explosion.setParticleRadius(0.05);
	explosion.setEmitterType(RadialEmitter);
	explosion.setGroupSize(1000);

	// Sets landing area
	validLandingArea = Box(Vector3(-24.8, -1.6, -18.6), Vector3(21.7, 16.1, 27.5));

	// Loads background image
	background.load("images/space.png");

	// Loads the font
	text.loadFont("arial.ttf", 15);

	// Sets initial boolean values for game logic
	standBy = true;
	gameOver = false;
	inBounds = false;
	showNearest = false;
	exploded = false;
}

//--------------------------------------------------------------
// incrementally update scene (animation)
//
void ofApp::update() {
	if (!gameOver && !standBy) {
		// Update positioning of lights
		keyLight.setPosition(keyLightPos);
		fillLight.setPosition(fillLightPos);
		rimLight.setPosition(rimLightPos);

		// Updates positioning of lander bounding box
		lander->updateBoundingBox();

		// Checks if lander is in bounds of valid landing area
		if (lander->shipBBox.overlap(validLandingArea)) {
			inBounds = true;
		}
		else {
			inBounds = false;
		}

		// Adds Impulse Force for ground collision
		this->checkCollisions();

		// Handles physics movement of ship
		lander->integrate();

		// Handles physics rotation of ship
		lander->integrateTurn();

		// Update cameras relative to current position of lander
		if (lander->getShipLoaded()) {
			top.setPosition(lander->getPosition());
			front.setPosition(ofVec3f(lander->getPosition().x, lander->getPosition().y + 5, lander->getPosition().z - 5));
			follow.setPosition(ofVec3f(lander->getPosition().x, lander->getPosition().y, lander->getPosition().z + 40));
			follow.lookAt(ofVec3f(lander->getPosition().x, lander->getPosition().y, lander->getPosition().z));
			ground.lookAt(ofVec3f(lander->getPosition().x, lander->getPosition().y, lander->getPosition().z));
		}

		// Updates the altitude variable
		ofVec3f p;
		raySelectLine(p);
		altitude = lander->getPosition().y - p.y;

		// Calls update on emitter for exhaust
		emitter.setPosition(ofVec3f(lander->getPosition().x, lander->getPosition().y + 2.5, lander->getPosition().z));
		emitter.setOneShot(true);
		emitter.setVelocity(ofVec3f(0, -25, 0));
		emitter.update();

		// Calls update on explosion emitter
		explosion.setPosition(ofVec3f(lander->getPosition().x, lander->getPosition().y + 2.5, lander->getPosition().z));
		explosion.setOneShot(true);
		explosion.setVelocity(ofVec3f(0, -25, 0));
		explosion.update();

		// Checks and Sets variables for game logic
		if (lander->landed)
			gameOver = true;
		if (exploded)
			lander->thrust = 0;		// Lander cannot move if it exploded
	}
}

// load vertex buffer in preparation for rendering
//
void ofApp::loadVbo() {
	if (emitter.sys->particles.size() < 1) return;

	vector<ofVec3f> sizes;
	vector<ofVec3f> points;
	for (int i = 0; i < emitter.sys->particles.size(); i++) {
		points.push_back(emitter.sys->particles[i].position);
		sizes.push_back(ofVec3f(5));
	}
	// upload the data to the vbo
	//
	int total = (int)points.size();
	vbo.clear();
	vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);
	vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);
}

//--------------------------------------------------------------
void ofApp::draw() {
	glDepthMask(false);
	// Sets default color
	ofSetColor(255, 255, 255);
	// Draws background image
	background.draw(0, 0);
	// Draws gui
	if (!bHide) gui.draw();
	glDepthMask(true);

	theCam->begin();
	ofPushMatrix();

	// Draw all the lights 
	if (!hideLights) {
		keyLight.draw();
		fillLight.draw();
		rimLight.draw();
	}

	// Draws bounding box of landing area
	ofSetColor(ofColor::lightBlue);
	ofNoFill();
	drawBox(validLandingArea);

	if (bWireframe) {	// wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		terrain.drawWireframe();
		if (lander->getShipLoaded()) {
			lander->getShipModel().drawWireframe();
			if (!bTerrainSelected) drawAxis(lander->getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}
	else {
		ofEnableLighting();		// shaded mode
		terrain.drawFaces();
		ofMesh mesh;
		if (lander->getShipLoaded()) {
			lander->getShipModel().drawFaces();
			if (!bTerrainSelected) drawAxis(lander->getPosition());
			if (bDisplayBBoxes) {
				ofNoFill();
				ofSetColor(ofColor::white);
				for (int i = 0; i < lander->getShipModel().getNumMeshes(); i++) {
					ofPushMatrix();
					ofMultMatrix(lander->getShipModel().getModelMatrix());
					ofRotate(-90, 1, 0, 0);
					Octree::drawBox(bboxList[i]);
				}
			}

			if (lander->getShipSelected()) {
				ofVec3f min = lander->getShipModel().getSceneMin() + lander->getPosition();
				ofVec3f max = lander->getShipModel().getSceneMax() + lander->getPosition();

				Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
				ofSetColor(ofColor::white);
				Octree::drawBox(bounds);

				// draw colliding boxes
				//
				ofNoFill();
				ofSetColor(ofColor::lightGreen);
				for (int i = 0; i < colBoxList.size(); i++) {
					Octree::drawBox(colBoxList[i]);
				}
			}
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));

		if (bDisplayPoints) {                // display points as an option    
			glPointSize(3);
			ofSetColor(ofColor::green);
			terrain.drawVertices();
		}

		// highlight selected point (draw sphere around selected point)
		//
		if (bPointSelected) {
			ofSetColor(ofColor::blue);
			ofDrawSphere(selectedPoint, .1);
		}


		// recursively draw octree
		//
		ofDisableLighting();
		int level = 0;
		//	ofNoFill();

		if (bDisplayLeafNodes) {
			octree.drawLeafNodes(octree.root);
			cout << "num leaf: " << octree.numLeaf << endl;
		}
		else if (bDisplayOctree) {
			ofNoFill();
			ofSetColor(ofColor::white);
			octree.draw(octree.root, numLevels, 0);
		}

		// if point selected, draw a sphere
		//
		if (pointSelected && showNearest) {
			ofVec3f p = octree.mesh.getVertex(selectedNode.points[0]);
			ofVec3f d = p - cam.getPosition();
			ofSetColor(ofColor::lightGreen);
			ofDrawSphere(p, .02 * d.length());

			if (showNearest)
				ofSetColor(ofColor::green);
			ofDrawLine(lander->getPosition(), p);


		}

	}

	// Draws Emitter for exhaust
	//emitter.draw();
	loadVbo();
	glDepthMask(GL_FALSE);
	ofSetColor(ofColor::red);
	// makes everything look glowy
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnablePointSprites();
	// begin shader
	shader.begin();
	// draw exhaust particle emitter
	particleTex.bind();
	vbo.draw(GL_POINTS, 0, (int)emitter.sys->particles.size());
	particleTex.unbind();
	// end drawing
	shader.end();
	ofDisablePointSprites();
	ofDisableBlendMode();
	glDepthMask(GL_TRUE);

	// Draws Emitter for explosion
	explosion.draw();


	ofPopMatrix();
	theCam->end();

	// Set text color to white
	ofSetColor(ofColor::green);

	if (gameOver) {	// Messages for game over conditions
		string goText, goText2;
		if (exploded) {
			goText = "You lose: landed too hard";
			text.drawString(goText, ofGetWindowWidth() / 2 - text.stringWidth(goText) / 2, ofGetWindowHeight() / 2 - 20);
			goText2 = "Push R to try again";
			text.drawString(goText2, ofGetWindowWidth() / 2 - text.stringWidth(goText2) / 2, ofGetWindowHeight() / 2 + 20);
		}
		else if (inBounds) {
			goText = "You landed!";
			text.drawString(goText, ofGetWindowWidth() / 2 - text.stringWidth(goText) / 2, ofGetWindowHeight() / 2 - 20);
			goText2 = "Push R to try again";
			text.drawString(goText2, ofGetWindowWidth() / 2 - text.stringWidth(goText2) / 2, ofGetWindowHeight() / 2 + 20);
		}
		else if (!inBounds) {
			goText = "You lose: did not land in correct area";
			text.drawString(goText, ofGetWindowWidth() / 2 - text.stringWidth(goText) / 2, ofGetWindowHeight() / 2 - 20);
			goText2 = "Push R to try again";
			text.drawString(goText2, ofGetWindowWidth() / 2 - text.stringWidth(goText2) / 2, ofGetWindowHeight() / 2 + 20);
		}
	}
	else if (standBy) {	// Message while in standby mode (before game starts)
		string goText, goText2, goText3, goText4;
		goText = ("Press Space to Begin");
		text.drawString(goText, ofGetWindowWidth() / 2 - text.stringWidth(goText) / 2, ofGetWindowHeight() / 2 - 35);
		goText2 = ("Move with WASD, Rotate with left and right arrow keys, Move up with Space Bar");
		text.drawString(goText2, ofGetWindowWidth() / 2 - text.stringWidth(goText2) / 2, ofGetWindowHeight() / 2);
		goText3 = ("F1 = default view, F2 = view from ship below, F3 = side view of ship");
		text.drawString(goText3, ofGetWindowWidth() / 2 - text.stringWidth(goText3) / 2, ofGetWindowHeight() / 2 + 35);
		goText4 = ("F4 = view from ship in front, F5 = view ship from ground level, E = altitude sensor display, c = enable freecam");
		text.drawString(goText4, ofGetWindowWidth() / 2 - text.stringWidth(goText4) / 2, ofGetWindowHeight() / 2 + 70);
	}
	ofSetColor(ofColor::white);
	// Displays altitude levels on screen
	string framerateText;
	framerateText += "Frame Rate: " + std::to_string(ofGetFrameRate());
	text.drawString(framerateText, ofGetWindowWidth() - 225, 25);
	// Displays fuel levels on screen
	string fuelText;
	fuelText += "Fuel Supply: " + std::to_string(lander->fuel);
	text.drawString(fuelText, ofGetWindowWidth() - 225, 50);
	// Displays altitude levels on screen
	string altitudeText;
	altitudeText += "Altitude: " + std::to_string(altitude);
	text.drawString(altitudeText, ofGetWindowWidth() - 190, 75);
}

// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(ofVec3f location) {

	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));


	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}


void ofApp::keyPressed(int key) {

	keymap[key] = true;
	if (keymap['J'] | keymap['j']) {	// Toggles display of gui
		bHide = !bHide;
	}
	if (keymap['L'] | keymap['l']) {	// Toggles display of lights
		hideLights = !hideLights;
	}
	if (keymap['B'] | keymap['b']) {
		bDisplayBBoxes = !bDisplayBBoxes;
	}
	if (keymap['C'] | keymap['c']) {
		if (cam.getMouseInputEnabled())
			cam.disableMouseInput();
		else
			cam.enableMouseInput();
	}
	if (keymap['E'] | keymap['e']) {
		showNearest = !showNearest;
	}
	if (keymap['F'] | keymap['f']) {
		ofToggleFullscreen();
	}
	if (keymap['O'] | keymap['o']) {	// Toggles display of octree
		bDisplayOctree = !bDisplayOctree;
	}
	if (keymap['R'] | keymap['r']) {	// Resets lander and game logic fields
		if (gameOver) {
			exploded = false;
			gameOver = false;
			inBounds = false;
			showNearest = false;
			lander->shipModel.loadModel("geo/lander.obj");
			lander->shipModel.setScaleNormalization(false);
			lander->thrust = 25.0;
			lander->acceleration = glm::vec3(0, 0, 0);
			lander->turnVelocity = 0;
			lander->velocity = glm::vec3(0, 0, 0);
			lander->turnVelocity = 0;
			lander->rotation = 0;
			lander->setPosition(ofVec3f(-50, 30, -50));
			lander->fuel = 200;
			lander->landed = false;
			lander->shipSelected = false;
		}
	}
	if (keymap['P'] | keymap['p']) {
		savePicture();
	}
	if (keymap['T'] | keymap['t']) {
		setCameraTarget();
	}
	if (keymap['V'] | keymap['v']) {
		togglePointsDisplay();
	}
	if (keymap['Q'] | keymap['q']) {
		toggleWireframeMode();
	}
	if (keymap[OF_KEY_CONTROL]) {
		bCtrlKeyDown = true;
	}
	if (keymap[OF_KEY_F1]) {	// switches to default easyCam
		theCam = &cam;
	}
	if (keymap[OF_KEY_F2]) {	// switches camera to top cam
		theCam = &top;
	}
	if (keymap[OF_KEY_F3]) {	// switches camera to following cam
		theCam = &follow;
	}
	if (keymap[OF_KEY_F4]) {	// switches camera to front cam
		theCam = &front;
	}
	if (keymap[OF_KEY_F5]) {	// switches camera to ground cam
		theCam = &ground;
	}
	if (keymap[' ']) {					// Move forward relative to Y-Axis
		// Only move if lander has fuel and lander has not exploded
		if (lander->fuel > 0 && !exploded) {
			// Play exhaust sound effect
			exhaust.play();
			// Starts exhaust emitter
			if (!emitter.started)
				emitter.start();
			lander->appliedThrust = lander->thrust * glm::vec3(0, 1, 0);
			// Decreases fuel
			lander->fuel--;
		}
		// Start game
		if (standBy) {
			standBy = !standBy;
		}
	}
	if (keymap['W'] | keymap['w']) {	// Move backward relative to Z-Axis 
		lander->appliedThrust = lander->thrust * glm::vec3(0, 0, -1);
	}
	if (keymap['S'] | keymap['s']) {	// Move forward relative to Z-Axis
		lander->appliedThrust = lander->thrust * glm::vec3(0, 0, 1);
	}
	if (keymap['D'] | keymap['d']) {	// Move forward relative to X-Axis
		lander->appliedThrust = lander->thrust * glm::vec3(1, 0, 0);
	}
	if (keymap['A'] | keymap['a']) {	// Move backward relative to X-Axis
		lander->appliedThrust = lander->thrust * glm::vec3(-1, 0, 0);
	}
	if (keymap[OF_KEY_LEFT]) {			// Rotate left about the Y-Axis
		lander->turnAcceleration = lander->thrust * -3;
	}
	if (keymap[OF_KEY_RIGHT]) {			// Rotate right about the Y-Axis
		lander->turnAcceleration = lander->thrust * 3;
	}
	if (keymap[OF_KEY_DOWN]) {			// Move forward relative to Y-Axis
		lander->appliedThrust = lander->thrust * glm::vec3(0, -1, 0);
	}
	if (keymap[OF_KEY_UP]) {			// Move backward relative to Y-Axis
		lander->appliedThrust = lander->thrust * glm::vec3(0, 1, 0);
	}
}

void ofApp::keyReleased(int key) {
	keymap[key] = false;
	if (!keymap[OF_KEY_CONTROL]) {
		bCtrlKeyDown = false;
	}
	if (!keymap[OF_KEY_SHIFT]) {

	}
	if (!keymap[' ']) {					// Stop applying thrust force
		lander->appliedThrust = glm::vec3(0, 0, 0);
	}
	if (!keymap['W'] | !keymap['w']) {	// Stop applying thrust force
		lander->appliedThrust = glm::vec3(0, 0, 0);
	}
	if (!keymap['S'] | !keymap['s']) {	// Stop applying thrust force
		lander->appliedThrust = glm::vec3(0, 0, 0);
	}
	if (!keymap['D'] | !keymap['d']) {	// Stop applying thrust force
		lander->appliedThrust = glm::vec3(0, 0, 0);
	}
	if (!keymap['A'] | !keymap['a']) {	// Stop applying thrust force
		lander->appliedThrust = glm::vec3(0, 0, 0);
	}
	if (!keymap[OF_KEY_LEFT]) {
		lander->turnAcceleration = 0;
	}
	if (!keymap[OF_KEY_RIGHT]) {
		lander->turnAcceleration = 0;
	}
	if (!keymap[OF_KEY_UP]) {
		lander->acceleration = glm::vec3(0, 0, 0);
	}
	if (!keymap[OF_KEY_DOWN]) {
		lander->acceleration = glm::vec3(0, 0, 0);
	}
}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {



}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	// implement you code here to select the rover
	// if Selected, draw box in a different color
	// check if the lander has been loaded 
	if (lander->getShipLoaded()) {
		// get mousePoint in 3D
		glm::vec3 origin = theCam->getPosition();
		glm::vec3 mouseWorld = theCam->screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

		ofVec3f min = lander->getShipModel().getSceneMin() + lander->getShipModel().getPosition();
		ofVec3f max = lander->getShipModel().getSceneMax() + lander->getShipModel().getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
		if (hit) {
			lander->shipSelected = true;
			ofPoint currentLanderPosition = lander->getShipModel().getPosition();
			mouseDownPos = getMousePointOnPlane(currentLanderPosition, cam.getZAxis());
			mouseLastPos = mouseDownPos;
			bInDrag = true;
		}
		else {
			lander->shipSelected = false;
		}
	}
	else {
		ofVec3f p;
		raySelectWithOctree(p);
	}
}

bool ofApp::raySelectWithOctree(ofVec3f & pointRet)
{
	ofVec3f mouse(mouseX, mouseY);
	ofVec3f rayPoint = theCam->screenToWorld(mouse);
	ofVec3f rayDir = rayPoint - theCam->getPosition();
	rayDir.normalize();
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
		Vector3(rayDir.x, rayDir.y, rayDir.z));

	pointSelected = octree.intersect(ray, octree.root, selectedNode);

	//printf("In Box: %d \n", pointSelected);

	if (pointSelected) {
		pointRet = octree.mesh.getVertex(selectedNode.points[0]);
	}
	return pointSelected;
}


// Shoots ray from top cam of lander to the moon terrain
// Uses octree to find the nearest valid point on terrain
// from top cam of lander
// --Jared Bechthold
bool ofApp::raySelectLine(ofVec3f & pointRet)
{
	ofVec3f landerScreenPos(ofGetWindowWidth() / 2, ofGetWindowHeight() / 2);
	ofVec3f rayPoint = top.screenToWorld(landerScreenPos);
	ofVec3f rayDir = rayPoint - top.getPosition();
	rayDir.normalize();
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
		Vector3(rayDir.x, rayDir.y, rayDir.z));

	pointSelected = octree.intersect(ray, octree.root, selectedNode);

	//printf("In Box: %d \n", pointSelected);

	if (pointSelected) {
		pointRet = octree.mesh.getVertex(selectedNode.points[0]);
		ofSetColor(ofColor::green);
		ofDrawLine(lander->getPosition(), pointRet);
	}
	return pointSelected;
}


//draw a box from a "Box" class  
//
void ofApp::drawBox(const Box &box) {
	Vector3 min = box.parameters[0];
	Vector3 max = box.parameters[1];
	Vector3 size = max - min;
	Vector3 center = size / 2 + min;
	ofVec3f p = ofVec3f(center.x(), center.y(), center.z());
	float w = size.x();
	float h = size.y();
	float d = size.z();
	ofDrawBox(p, w, h, d);
}

// return a Mesh Bounding Box for the entire Mesh
//
Box ofApp::meshBounds(const ofMesh & mesh) {
	int n = mesh.getNumVertices();
	ofVec3f v = mesh.getVertex(0);
	ofVec3f max = v;
	ofVec3f min = v;
	for (int i = 1; i < n; i++) {
		ofVec3f v = mesh.getVertex(i);

		if (v.x > max.x) max.x = v.x;
		else if (v.x < min.x) min.x = v.x;

		if (v.y > max.y) max.y = v.y;
		else if (v.y < min.y) min.y = v.y;

		if (v.z > max.z) max.z = v.z;
		else if (v.z < min.z) min.z = v.z;
	}
	return Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	if (bInDrag && lander->getShipLoaded()) {

		glm::vec3 landerPos = lander->getPosition();

		glm::vec3 mousePos = getMousePointOnPlane(landerPos, cam.getZAxis());
		glm::vec3 delta = mousePos - mouseLastPos;

		landerPos += delta;
		lander->setPosition(landerPos);
		mouseLastPos = mousePos;

		ofVec3f min = lander->getShipModel().getSceneMin() + lander->getShipModel().getPosition();
		ofVec3f max = lander->getShipModel().getSceneMax() + lander->getShipModel().getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

		colBoxList.clear();
		octree.intersect(lander->shipBBox, octree.root, colBoxList);

		//printf("Intersects? %d\n", octree.intersect(lander->shipBBox, octree.root, colBoxList));
		//printf("boxes: %d \n", colBoxList.size());



		/*if (bounds.overlap(testBox)) {
			cout << "overlap" << endl;
		}
		else {
			cout << "OK" << endl;
		}*/


	}
	else {
		ofVec3f p;
		float startTime = ofGetElapsedTimeMillis();
		raySelectWithOctree(p);
		float raySelectTime = ofGetElapsedTimeMillis() - startTime;
		printf("Time to search tree with ray intersection: %fms\n", raySelectTime);
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bInDrag = false;
}



// Set the camera to use the selected point as it's new target
//  
void ofApp::setCameraTarget() {

}


//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}



//--------------------------------------------------------------
// setup basic ambient lighting in GL  (for now, enable just 1 light)
//
void ofApp::initLightingAndMaterials() {

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{ 5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
}

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point) {
	ofVec2f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}


//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
	// Setup our rays
	//
	glm::vec3 origin = cam.getPosition();
	glm::vec3 camAxis = cam.getZAxis();
	glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		// We use the parameteric line or vector representation of a line to compute
		//
		// p' = p + s * dir;
		//
		glm::vec3 intersectPoint = origin + distance * mouseDir;

		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}


// Checks if lander collided with surface and instantiates
// the lander's impulse force if it did
// Depending on value of impulse force, lander may land or blow up
// --Jared Bechthold
void ofApp::checkCollisions()
{
	// Checks if lander collides with ground and is falling
	colBoxList.clear();
	if (octree.intersect(lander->shipBBox, octree.root, colBoxList) && lander->velocity.y < 0) {
		ofVec3f norm = ofVec3f(0, 1, 0);
		ofVec3f vel = lander->velocity;

		// Sets lander's impulse force
		lander->impulseForce = 60 * (1.85)*((-vel.dot(norm))*norm);
		// Checks if lander is in bounds and below specific impulse force
		if (lander->impulseForce.y < 500 && lander->impulseForce.y > 0) {
			// if lander lands sets gameOver to false and empties thrust
			if (gameOver == false) {
				lander->thrust = 0;
				lander->landed = true;
			}
		}
		// Checks if lander's impulse force is above specific value
		else if (lander->impulseForce.y > 800) {
			// Triggers explosion
			if (!explosion.started) {
				lander->shipModel.clear();
				boom.play();
				explosion.start();
				exploded = true;
			}
		}
	}
}

// Constructor for Ship instance
// --Jared Bechthold
//----------------------------------------------------
Ship::Ship(string src)
{
	// loads ship model from given source
	if (shipModel.loadModel(src))
		shipLoaded = true;
	shipModel.setScaleNormalization(false);

	// sets up bounding box of ship
	ofVec3f min = getShipModel().getSceneMin() + getPosition();
	ofVec3f max = getShipModel().getSceneMax() + getPosition();
	shipBBox = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}

// Applies forces to Ship instance and changes position appropriately
// This gives the Ship instance physics movement
// --Jared Bechthold
//----------------------------------------------------
void Ship::integrate()
{
	// update position from velocity and time
	this->setPosition(this->getPosition() + this->velocity * 1.0 / 60.0);
	// adds forces
	addForces();
	ofVec3f accel = acceleration + forces;
	//printf("%f, %f, %f\n", forces.x, forces.y, forces.z);
	// update velocity (from acceleration)
	this->velocity = this->velocity + accel * (1.0 / 60.0);
	// multiply velocity by damping factor
	this->velocity = this->velocity * this->damping;
	//printf("%f, %f, %f\n", velocity.x, velocity.y, velocity.z);
	impulseForce.set(0, 0, 0);
	forces.set(0, 0, 0);
}

// Applies thrust force to Ship instances rotation
// --Jared Bechthold
//----------------------------------------------------
void Ship::integrateTurn()
{
	// update rotation from velocity and time
	this->rotation = this->rotation + this->turnVelocity * 1.0 / 60.0;
	this->setRotation();
	// update velocity (from acceleration)
	this->turnVelocity = this->turnVelocity + this->turnAcceleration * (1.0 / 60.0);
	// multiply velocity by damping factor
	this->turnVelocity = this->turnVelocity * this->damping;
}

// Sets position of Ship model and stores in Ship position field
// --Jared Bechthold
//----------------------------------------------------
void Ship::setPosition(glm::vec3 newPos)
{
	if (getShipLoaded()) {
		position = newPos;
		shipModel.setPosition(newPos.x, newPos.y, newPos.z);
	}
}

// Sets rotation angle of Ship model to current value of rotation variable
// --Jared Bechthold
//----------------------------------------------------
void Ship::setRotation()
{
	if (getShipLoaded())
		shipModel.setRotation(0, this->rotation, 0.0, 1.0, 0.0);
}

// Returns boolean value if ship is selected
// --Jared Bechthold
//----------------------------------------------------
bool Ship::getShipSelected()
{
	return shipSelected;
}

// Returns boolean value if ship is loaded
// --Jared Bechthold
//----------------------------------------------------
bool Ship::getShipLoaded()
{
	return shipLoaded;
}

// Returns the ship model's bounding box
// --Jared Bechthold
//----------------------------------------------------
Box Ship::getLanderBounds()
{
	if (getShipLoaded())
		return shipBBox;
}

// Returns the ship's model
// --Jared Bechthold
//----------------------------------------------------
ofxAssimpModelLoader Ship::getShipModel()
{
	if (getShipLoaded())
		return shipModel;
}

// Returns the ship's current position
// --Jared Bechthold
//----------------------------------------------------
glm::vec3 Ship::getPosition()
{
	return position;
}

// Adds all of the force vectors to the forces vector
// --Jared Bechthold
//----------------------------------------------------
void Ship::addForces()
{
	forces = gravity + appliedThrust + impulseForce;
}

// Updates the position of the ship model's bounding box
// --Jared Bechthold
//----------------------------------------------------
void Ship::updateBoundingBox()
{
	if (getShipLoaded()) {
		ofVec3f min = getShipModel().getSceneMin() + getPosition();
		ofVec3f max = getShipModel().getSceneMax() + getPosition();

		shipBBox = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
	}
}

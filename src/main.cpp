#include "MyViewer.cpp"
#include "boids/BoidsViewer.cpp"
#include "Particles/ParticlesViewer.cpp"
#include "FK/FKViewer.cpp"
#include "cloth/ClothViewer.cpp"
#include "bounce/BounceViewer.cpp"
#include "Spider/SpiderViewer.cpp"

int main(int argc, char** argv) {
	//MyViewer v;
	//BoidsViewer v;
	//ParticlesViewer v;
	//ClothViewer v;
	//FKViewer v;
	//FabrikViewer v;
	//BounceViewer v;
	SpiderViewer v;
	return v.run();
}

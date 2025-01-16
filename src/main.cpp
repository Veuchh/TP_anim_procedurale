#include "MyViewer.cpp"
#include "boids/BoidsViewer.cpp"
#include "Particles/ParticlesViewer.cpp"
#include "FK/FKViewer.cpp"
#include "cloth/ClothViewer.cpp"
#include "bounce/BounceViewer.cpp"

int main(int argc, char** argv) {
	//MyViewer v;
	//BoidsViewer v;
	//ParticlesViewer v;
	//ClothViewer v;
	//FKViewer v;
	BounceViewer v;
	return v.run();
}

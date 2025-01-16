#include "MyViewer.cpp"
#include "boids/BoidsViewer.cpp"
#include "Particles/ParticlesViewer.cpp"
#include "FK/FKViewer.cpp"
#include "Fabrik/FabrikViewer.cpp"
#include "cloth/ClothViewer.cpp"

int main(int argc, char** argv) {
	//MyViewer v;
	//BoidsViewer v;
	//ParticlesViewer v;
	//FKViewer v;
	//ClothViewer v;
	FabrikViewer v;
	return v.run();
}

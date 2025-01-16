#include "MyViewer.cpp"
#include "boids/BoidsViewer.cpp"
#include "Particles/ParticlesViewer.cpp"
#include "FK/FKViewer.cpp"

int main(int argc, char** argv) {
	//MyViewer v;
	//BoidsViewer v;
	//ParticlesViewer v;
	FKViewer v;
	return v.run();
}

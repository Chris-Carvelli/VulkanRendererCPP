#include <VKRenderer.hpp>

class TestRenderer : public VKRenderer {
};

int main() {
	TestRenderer app;

	app.run();

	return 0;
}
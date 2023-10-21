
struct SceneInfo {
	const char* scenePath;

	SceneInfo(const char* scenePath) 
		: scenePath(scenePath) {

	}
};

class Scene {
public:
	Scene(SceneInfo info);
	
	void cleanup();
};
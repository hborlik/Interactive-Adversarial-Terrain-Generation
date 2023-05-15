#include <graphics/camera.h>

using namespace dirtbox::input;
using namespace std;

namespace dirtbox {


Camera::Camera()
{
    reset();
    update(0.0f, MouseState{});

    vector<shared_ptr<InputBinding>> vecfn = {};
    
    vecfn.push_back(std::make_shared<InputBinding>(
        Key::KeyW, 
        Modifier::Empty,
        0,
        InputBindingFn::create<Camera, &Camera::forward>(this)));
    vecfn.push_back(std::make_shared<InputBinding>(
        Key::KeyA, 
        Modifier::Empty,
        0,
        InputBindingFn::create<Camera, &Camera::left>(this)));
    vecfn.push_back(std::make_shared<InputBinding>(
        Key::KeyS, 
        Modifier::Empty,
        0,
        InputBindingFn::create<Camera, &Camera::backward>(this)));
    vecfn.push_back(std::make_shared<InputBinding>(
        Key::KeyD, 
        Modifier::Empty,
        0,
        InputBindingFn::create<Camera, &Camera::right>(this)));
    vecfn.push_back(std::make_shared<InputBinding>(
        Key::KeyQ, 
        Modifier::Empty,
        0,
        InputBindingFn::create<Camera, &Camera::down>(this)));
    vecfn.push_back(std::make_shared<InputBinding>(
        Key::KeyE, 
        Modifier::Empty,
        0,
        InputBindingFn::create<Camera, &Camera::up>(this)));
        // };

    inputAddBindings("camInput", vecfn);
}

Camera::~Camera() {
    inputRemoveBindings("camInput");
}


    
}
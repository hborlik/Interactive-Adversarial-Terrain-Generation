/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-08
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_SCENE_H
#define DIRTBOX_SCENE_H

#include <vector>
#include <memory>
#include <string>

namespace dirtbox::core {

class Scene;

class Node {
public:
    Node() = default;
    explicit Node(const std::string& name);
    virtual ~Node() {}

    const std::string& getName() const {return name;}
    
private:
    std::string name;
};

class Scene {
public:

    void add_node(std::shared_ptr<Node> n);
    void remove_node(std::shared_ptr<Node> n);

private:
    std::vector<std::shared_ptr<Node>> nodes;
};

}

#endif // DIRTBOX_SCENE_H
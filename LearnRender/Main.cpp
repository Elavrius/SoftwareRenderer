#include <vector> 
#include <iostream> 
#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include <SDL.h>
#include <stdio.h>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const int width = 1000;
const int height = 1000;
const int depth = 255;

void line2(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) { // if the line is steep, we transpose the image
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) { // make it left-to-right
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    int yAdd = y1 > y0 ? 1 : -1;
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
        error2 += derror2;

        if (error2 > dx) {
            y += yAdd;
            error2 -= dx * 2;
        }
    }
}

void line(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color) {
    bool steep = false;
    if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
        steep = true;
    }
    if (p0.x > p1.x) {
        std::swap(p0, p1);
    }

    for (int x = p0.x; x <= p1.x; x++) {
        float t = (x - p0.x) / (float)(p1.x - p0.x);
        int y = p0.y * (1. - t) + p1.y * t;
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
    }
}

void horizontalLine(int y, Vec3i A, Vec3i B, SDL_Renderer* renderer, TGAColor color, int* zbuffer) {
    if (A.x > B.x) std::swap(A,B);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int x = A.x; x <= B.x; x++) {
        float phi = B.x == A.x ? 1. : (float)(x - A.x) / (float)(B.x - A.x);
        Vec3i complimentary = A - B;
        Vec3f P = Vec3f(A.x, A.y, A.z) + Vec3f(complimentary.x, complimentary.y, complimentary.z) * phi;
        Vec3i Z = Vec3i(P.x, P.y, P.z);
        int idx = Z.x + Z.y * width;
        if (zbuffer[idx] < Z.z) {
            zbuffer[idx] = Z.z;
            SDL_RenderDrawPoint(renderer, x, y);
        }
        
    }
}
 
void triangle(Vec3i t0, Vec3i t1, Vec3i t2, SDL_Renderer* renderer, TGAColor color, int *zbuffer) {
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    int total_height = t2.y - t0.y+1;
    for (int y = t0.y; y <= t1.y; y++) {
        int segment_height = t1.y - t0.y + 1;
        float alpha = (float)(y - t0.y) / total_height;
        float beta = (float)(y - t0.y) / segment_height; // be careful with divisions by zero 
        Vec3i A = t0 + ((t2 - t0) * alpha);
        Vec3i B = t0 + ((t1 - t0) * beta);
        horizontalLine(y, A, B, renderer, color, zbuffer);
    }
    for (int y = t1.y+1; y <= t2.y; y++) {
        int segment_height = t2.y - t1.y + 1;
        float alpha = (float)(y - t0.y) / total_height;
        float beta = (float)(y - t1.y) / segment_height; // be careful with divisions by zero 
        Vec3i A = t0 + ((t2 - t0) * alpha);
        Vec3i B = t1 + ((t2 - t1) * beta);
        horizontalLine(y, A, B, renderer, color, zbuffer);
    }
}

int main(int argc, char** argv) {

    Model* model = new Model("obj/african_head.obj");

    SDL_Event event;
    SDL_Renderer* renderer;
    SDL_Window* window;
    int i;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(height, width, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    int count = 0;

    int *zbuffer = new int[width * height];
    Vec3f light_dir(0, 0, -1);
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3i screen_coords[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec3i((v.x + 1.) * width / 2., height - (v.y + 1.) * height / 2., (v.z + 1.) *depth/2);
            world_coords[j] = v;
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            triangle(screen_coords[0], screen_coords[1], screen_coords[2], renderer, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255), zbuffer);
            count++;
        }
    }


        SDL_RenderPresent(renderer);
        while (1) {
            if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
                break;
        }
        delete model;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
    
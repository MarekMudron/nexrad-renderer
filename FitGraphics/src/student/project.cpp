/**
 * This file can be used as template for student project.
 *
 * There is a lot of hidden functionality not documented
 * If you have questions about the framework
 * email me: imilet@fit.vutbr.cz
 */

#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <Vars/Vars.h>
#include <geGL/geGL.h>
#include <geGL/StaticCalls.h>
#include <imguiDormon/imgui.h>
#include <imguiVars/addVarsLimits.h>
#include <framework/FunctionPrologue.h>
#include <BasicCamera/OrbitCamera.h>
#include <BasicCamera/PerspectiveCamera.h>
#include <framework/methodRegister.hpp>
#include <framework/makeProgram.hpp>
#include <PGR/02/model.hpp>
#include <iostream>
#include <Radx/RadxFile.hh>
#include <Radx/NexradRadxFile.hh>
#include <Radx/RadxVol.hh>
#include <Radx/RadxRay.hh>
#include "dataloader.hpp"
#include <random>
#include <filesystem>
namespace fs = std::filesystem;

using namespace ge::gl;
using namespace std;
#define PI 3.1415926538
namespace student::project
{

  std::shared_ptr<Program> prg;
  std::shared_ptr<VertexArray> vao;
  std::shared_ptr<Buffer> vbo;

  std::string source = R".(
    #line 43
#ifdef VERTEX_SHADER

#define PI 3.1415926538
uniform mat4 model = mat4(1.f);
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform float heightExagg = 1.f;

layout(location=0)in float x;
layout(location=1)in float y;
layout(location=2)in float z;

out vec3 vCoord;


void main()
{
    gl_Position = proj*view*model*vec4(x, y, z*heightExagg, 1.0);
    vCoord = vec3(x,y,z);
}
#endif




#ifdef FRAGMENT_SHADER

layout(location=0)out vec4 fColor;
layout(binding=0)uniform sampler3D tex;
uniform float cutoff  = 0.f;
in vec3 vCoord;

#define MAX_DIST_KM  (2.125 + 0.25 *1832)

uniform float opacity = 1.f;

float toTex(float coord) {
  return (1 / MAX_DIST_KM * 2)*coord + 0.5f;
}



void main(){
  fColor= texture(tex, vec3(toTex(vCoord.x),toTex(vCoord.y),toTex(vCoord.z)));
  if(fColor.w < cutoff) {
    fColor.w = 0.f;
  }else{
    fColor.w = opacity;
  }
  
}
#endif
).";

  void setVertexAttrib(
      GLuint vao,
      GLuint attrib,
      GLint size,
      GLenum type,
      GLuint buffer,
      GLintptr offset,
      GLsizei stride)
  {
    glVertexArrayAttribBinding(
        vao,
        attrib,  // attrib index
        attrib); // binding index

    glEnableVertexArrayAttrib(
        vao,
        attrib); // attrib index

    glVertexArrayAttribFormat(
        vao,
        attrib, // attrib index
        size,
        type,
        GL_FALSE, // normalization
        0);       // relative offset

    glVertexArrayVertexBuffer(
        vao,
        attrib, // binding index
        buffer,
        offset,
        stride);
  }

  DataLoader dl;

  glm::vec3 to3d(float az, float elev, float dist)
  {
    az = PI * az / 180.f;
    elev = PI * elev / 180.f;
    float x = dist * cos(elev) * cos(az);
    float y = dist * cos(elev) * sin(az);
    float z = dist * sin(elev);

    return glm::vec3(x, y, z);
  }

  class TextureData
  {
  public:
    std::vector<float> data;
    uint32_t sizeX = 0;
    uint32_t sizeY = 0;
    uint32_t sizeZ = 0;
    TextureData() {}
    TextureData(uint32_t sX, uint32_t sY, uint32_t sZ) : sizeX(sX), sizeY(sY), sizeZ(sZ)
    {
      data.resize((size_t)(sizeX * sizeY * sizeZ * 4), 0.f);
    }
  };

  glm::vec3 hsv2rgb(const glm::vec3 &hsv)
  {
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;

    int i;
    float f, p, q, t;

    if (s == 0)
    {
      // Achromatic (gray)
      return glm::vec3(v, v, v);
    }

    h /= 60; // sector 0 to 5
    i = static_cast<int>(std::floor(h));
    f = h - i; // factorial part of h
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i)
    {
    case 0:
      return glm::vec3(v, t, p);
    case 1:
      return glm::vec3(q, v, p);
    case 2:
      return glm::vec3(p, v, t);
    case 3:
      return glm::vec3(p, q, v);
    case 4:
      return glm::vec3(t, p, v);
    default: // case 5:
      return glm::vec3(v, p, q);
    }
  }

  struct Texel
  {
    float x;
    float y;
    float z;
    float val;
  };

  TextureData loadTexture(DataLoader &dl, FieldChoice choice)
  {
    int32_t sizeX, sizeY, sizeZ;
    sizeX = 128 * 2;
    sizeY = 128 * 2;
    sizeZ = 128 * 2;
    TextureData res(sizeX, sizeY, sizeZ);

    auto measurements = dl.getMeasurement(choice);
    vector<Texel> texels;

    for_each(measurements.begin(), measurements.end(), [&](Measurement m)
             {
      auto pos3d = to3d(m.az, m.elev, m.dist);
      texels.push_back({pos3d.x, pos3d.y, pos3d.z,m.value}); });

    float MAX_DIST_KM = (2.125 + 0.25 * 1832);
    for_each(texels.begin(), texels.end(), [&](Texel t)
             {
      float meas0to400 = t.val*400.f/240.f + 200.f;
      glm::vec3 color = hsv2rgb(glm::vec3(meas0to400, 1.f,1.f));
      int x = 128.f * t.x/MAX_DIST_KM + 128;
      int y = 128.f * t.y/MAX_DIST_KM + 128;
      int z = 128.f * t.z/MAX_DIST_KM + 128;
      uint8_t val = static_cast<uint8_t>(t.val);
      int index = ((z * sizeY + y) * sizeX + x)*4;

      if (index < res.data.size())
      {
        res.data[index + 0] = color.r;
        res.data[index + 1] = color.g;
        res.data[index + 2] = color.b;
        res.data[index + 3] = meas0to400/400.f;
      } });

    return res;
  }

  GLuint createTexture(DataLoader dl, FieldChoice c)
  {
    auto texData = loadTexture(dl, c);

    GLuint res;
    glCreateTextures(GL_TEXTURE_3D, 1, &res);
    GLenum internalFormat = GL_RGBA;
    GLenum format = GL_RGBA;

    glTextureImage3DEXT(
        res,            // texture
        GL_TEXTURE_3D,  // target
        0,              // mipmap level
        internalFormat, // gpu format
        texData.sizeX,
        texData.sizeY,
        texData.sizeZ,
        0,                    // border
        format,               // cpu format
        GL_FLOAT,             // cpu type
        texData.data.data()); // pointer to data
    glGenerateTextureMipmap(res);
    glTextureParameteri(res, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(res, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(res, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(res, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(res, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return res;
  }

  GLuint tex;

  struct PointAround
  {
    float x;
    float y;
    float z;
    int row;
  };

  void loadRays(DataLoader &dl)
  {
    auto rays = dl.getRays();
    rays.push_back(rays[0]);
    vector<PointAround> pointsAround;
    for_each(rays.begin(), rays.end(), [&](Ray r)
             {
      glm::vec3 v = to3d(r.az, r.elev, r.radMax);
      pointsAround.push_back({v.x, v.y, v.z, r.row}); });
    vector<glm::vec3> trianglePoints;

    auto fp = pointsAround[0];
    int lastRow = -1;

    for (int i = 1; i < pointsAround.size(); i++)
    {
      trianglePoints.push_back({0, 0, 0});
      auto a = pointsAround.at(i - 1);
      auto b = pointsAround.at(i);
      if (lastRow != b.row)
      {
        trianglePoints.push_back({a.x, a.y, a.z});
        trianglePoints.push_back({fp.x, fp.y, fp.z});
        fp = b;
        lastRow = b.row;
        continue;
      }
      trianglePoints.push_back({a.x, a.y, a.z});
      trianglePoints.push_back({b.x, b.y, b.z});
    }

    vao = std::make_shared<VertexArray>();
    vbo = std::make_shared<Buffer>(
        trianglePoints.size() * sizeof(glm::vec3),
        trianglePoints.data(),
        GL_STATIC_DRAW);
    setVertexAttrib(vao->getId(), 0, 1, GL_FLOAT, vbo->getId(), 0, sizeof(glm::vec3));
    setVertexAttrib(vao->getId(), 1, 1, GL_FLOAT, vbo->getId(), sizeof(float), sizeof(glm::vec3));
    setVertexAttrib(vao->getId(), 2, 1, GL_FLOAT, vbo->getId(), 2 * sizeof(float), sizeof(glm::vec3));
    glVertexArrayElementBuffer(vao->getId(), vbo->getId());
  }

  std::vector<string> modelPaths;
  int currentIndex = 0;

  void initDirs()
  {
    std::string path = "../data";
    for (const auto &entry : fs::directory_iterator(path))
    {
      modelPaths.push_back(entry.path());
    }
  }

  GLuint loadTex(int index, FieldChoice c) {
    dl.loadVol(modelPaths.at(index));
    tex = createTexture(dl, c);
    return tex;
  }

  void onInit(vars::Vars &vars)
  {
    initDirs();
    tex = loadTex(0, FieldChoice::REF);
    loadRays(dl);

    model::setUpCamera(vars);
    prg = std::make_shared<Program>(
        std::make_shared<Shader>(GL_VERTEX_SHADER, "#version 460\n#define   VERTEX_SHADER\n" + source),
        std::make_shared<Shader>(GL_FRAGMENT_SHADER, "#version 460\n#define FRAGMENT_SHADER\n" + source));

    glClearColor(0, 0, 0, 1);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vars.get<basicCamera::OrbitCamera>("method.view")->setDistance(5.f);
    vars.get<basicCamera::PerspectiveCamera>("method.proj")->setNear(0.01);
    vars.get<basicCamera::PerspectiveCamera>("method.proj")->setFar(10000);
    vars.addFloat("data.opacity", 0.2f);
    vars.addFloat("data.cutoff", -120.f);
    vars.addFloat("data.heightExagg", 1.f);
    vars.addEnum("data.field", REF);
    addEnumValues<FieldChoice>(vars, {REF, VEL}, {"REF", "VEL"});
  }

  void onDraw(vars::Vars &vars)
  {
    auto choice = vars.getEnum<FieldChoice>("data.field");
    if (dl.needReload(choice))
    {
      tex = createTexture(dl, choice);
    }
    model::computeProjectionMatrix(vars);
    auto model = glm::mat4(1.f);
    auto view = vars.get<basicCamera::OrbitCamera>("method.view")->getView();
    auto proj = vars.get<basicCamera::PerspectiveCamera>("method.proj")->getProjection();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    prg->use();
    vao->bind();
    prg->setMatrix4fv("model", glm::value_ptr(model))
        ->setMatrix4fv("view", glm::value_ptr(view))
        ->setMatrix4fv("proj", glm::value_ptr(proj))
        ->set1f("opacity", *vars.get<float>("data.opacity"))
        ->set1f("heightExagg", *vars.get<float>("data.heightExagg"))
        ->set1f("cutoff", (*vars.get<float>("data.cutoff") * 400.f / 240.f + 200.f) / 400);
    glBindTextureUnit(0, tex);
    glDrawArrays(GL_TRIANGLES, 0, dl.numRays() * 3);

    vao->unbind();
  }

  void onResize(vars::Vars &vars)
  {
    // size of the screen
    auto width = vars.getUint32("event.resizeX");
    auto height = vars.getUint32("event.resizeY");

    glViewport(0, 0, width, height);
  }

  void onQuit(vars::Vars &vars)
  {
    vbo = nullptr;
    prg = nullptr;
    vao = nullptr;
    glDeleteTextures(1, &tex);
    vars.erase("method");
    vars.erase("data");
  }

  void onKeyUp(vars::Vars &vars)
  {
    auto key = vars.getInt32("event.key");
    if (key == SDLK_RIGHT)
    {
      if(++currentIndex >= modelPaths.size()) {
        currentIndex = 0;
      }
      tex = loadTex(currentIndex,vars.getEnum<FieldChoice>("data.field"));
    }
    else if (key == SDLK_LEFT)
    {
      if(--currentIndex < 0) {
        currentIndex = modelPaths.size() - 1;
      }
      tex = loadTex(currentIndex,vars.getEnum<FieldChoice>("data.field"));
    }
  }

  EntryPoint main = []()
  {
    /// table of callbacks
    methodManager::Callbacks clbs;
    clbs.onDraw = onDraw;
    clbs.onInit = onInit;
    clbs.onQuit = onQuit;
    clbs.onResize = onResize;
    clbs.onMouseMotion = model ::onMouseMotion;
    /// register method
    clbs.onKeyUp = onKeyUp;
    MethodRegister::get().manager.registerMethod("student.project", clbs);
  };
}

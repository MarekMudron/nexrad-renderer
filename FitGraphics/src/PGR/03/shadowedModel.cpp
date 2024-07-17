#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<framework/methodRegister.hpp>
#include<BasicCamera/OrbitCamera.h>
#include<BasicCamera/PerspectiveCamera.h>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/02/model.hpp>
#include<PGR/03/phong.hpp>
#include<framework/model.hpp>
#include<framework/FunctionPrologue.h>

#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/quaternion.hpp>
#include<glm/gtx/quaternion.hpp>

#include<libs/tiny_gltf/tiny_gltf.h>


using namespace ge::gl;

#ifndef CMAKE_ROOT_DIR
#define CMAKE_ROOT_DIR "."
#endif

namespace shadowedModel{

GLuint shadowMap;
GLuint fbo;

std::string const shadowMappingFS = R".(

uniform vec3  lightPosition = vec3(30,30,30)   ;
uniform vec3  lightColor    = vec3(1,1,1)      ;
uniform vec3  lightAmbient  = vec3(0.3,0.1,0.0);
uniform float shininess     = 60.f             ;

layout(location=0)out vec4 fColor;

layout(binding=0)uniform sampler2DShadow shadowMap;

in vec4 vShadowPos;
in vec3 vPosition;
in vec3 vNormal;
in vec2 vCoord;
in vec3 vCamPosition;

uniform sampler2D diffuseTexture;
uniform vec4      diffuseColor = vec4(1.f);
uniform int       useTexture   = 0;

void main(){
  float litValue = textureProj(shadowMap,vShadowPos).x;
  litValue = 1.f;

  vec3 diffuseColor = vec3(0.3);

  if(useTexture == 1)
    diffuseColor = texture(diffuseTexture,vCoord).rgb;
  else
    diffuseColor = diffuseColor.rgb;

  vec3 finalColor = phongLighting(
      vPosition          ,
      normalize(vNormal) ,
      lightPosition      ,
      vCamPosition       ,
      lightColor*litValue,
      lightAmbient       ,
      diffuseColor       ,
      shininess          ,
      0.f                );


  fColor = vec4(finalColor,1);
}

).";


class GModel{
  public:
    ~GModel(){
      meshes.clear();
      buffers.clear();
      textures.clear();
      roots.clear();
    }
    struct Node{
      glm::mat4        modelMatrix = glm::mat4(1.f);
      int32_t          mesh = -1;                   
      std::vector<Node>children;                    
    };
    struct Mesh{
      std::shared_ptr<ge::gl::VertexArray>vao;

      GLenum   indexType   = GL_UNSIGNED_INT;
      uint32_t nofIndices  =               0;
      uint64_t indexOffset =               0;
      bool     hasIndices  =           false;

      glm::vec4    diffuseColor = glm::vec4(1.f)  ;
      int          diffuseTexture = -1            ;
    };

    GModel(){}
    void load(std::string const&fileName){
      tinygltf::Model model;
      tinygltf::TinyGLTF loader;
      std::string err;
      std::string warn;
      bool ret;
      if(fileName.find(".glb")==fileName.length()-4)
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, fileName.c_str());
  
      if(fileName.find(".gltf")==fileName.length()-5)
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, fileName.c_str());
  
      if(!ret){
        std::cerr << "model: " << fileName << "was not be loaded" << std::endl;
        return;
      }
  
      auto const&scene = model.scenes.at(0);
      for(auto const&node_id:scene.nodes){
        auto const&root = model.nodes.at(node_id);
        roots.push_back(loadNode(root,model));
      }

      for(auto const&img:model.images){
        GLenum internalFormat = GL_RGB;
        if(img.component == 3)internalFormat = GL_RGB ;
        if(img.component == 4)internalFormat = GL_RGBA;
        auto tex = std::make_shared<ge::gl::Texture>(GL_TEXTURE_2D,internalFormat,0,img.width,img.height);
        tex->setData2D(img.image.data());
        tex->generateMipmap();
        textures.push_back(tex);
      }

      for(auto const&b:model.buffers)
        buffers.push_back(std::make_shared<ge::gl::Buffer>(b.data));

      for(auto const&mesh:model.meshes){
        for(auto const&primitive:mesh.primitives){
          if(primitive.mode != TINYGLTF_MODE_TRIANGLES)continue;

          meshes.push_back({});
          auto&m_mesh = meshes.back();
          m_mesh.vao = std::make_shared<ge::gl::VertexArray>();

          if (primitive.material >= 0) {
              auto const& mat = model.materials.at(primitive.material);
              auto baseColorTextureIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
              for (size_t i = 0; i < mat.pbrMetallicRoughness.baseColorFactor.size(); ++i)
                  m_mesh.diffuseColor[(uint32_t)i] = (float)mat.pbrMetallicRoughness.baseColorFactor.at(i);
            if(baseColorTextureIndex<0){
              m_mesh.diffuseTexture = -1;
            }else{
              m_mesh.diffuseTexture = model.textures.at(mat.pbrMetallicRoughness.baseColorTexture.index).source;
            }
          }else
            m_mesh.diffuseTexture = -1;

          if(primitive.indices >= 0){
              auto const&ia  = model.accessors.at(primitive.indices);
              auto const&ibv = model.bufferViews.at(ia.bufferView);
              m_mesh.vao->addElementBuffer(buffers.at(ibv.buffer));
              m_mesh.indexOffset = ibv.byteOffset + ia.byteOffset;
              m_mesh.nofIndices  = (uint32_t)ia.count;
              m_mesh.hasIndices  = true;
              if(ia.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT  )m_mesh.indexType = GL_UNSIGNED_INT  ;
              if(ia.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)m_mesh.indexType = GL_UNSIGNED_SHORT;
              if(ia.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE )m_mesh.indexType = GL_UNSIGNED_BYTE ;
          }else{
            //std::cerr << "dont have indices" << std::endl;
          }

          for(auto const&attrib:primitive.attributes){

            VertexAttrib*att = nullptr;
            auto const&accessor = model.accessors.at(attrib.second);

            auto addAttrib = [&](uint32_t att){
              auto const&bufferView = model.bufferViews.at(accessor.bufferView);
              auto bufId  = bufferView.buffer;
              auto stride = bufferView.byteStride;
              auto offset = bufferView.byteOffset+accessor.byteOffset;
              auto size   = bufferView.byteLength;
              auto bptr   = model.buffers.at(bufId).data.data() + accessor.byteOffset;
              uint32_t nofC;

              if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){
                if(accessor.type == TINYGLTF_TYPE_SCALAR)nofC = 1;
                if(accessor.type == TINYGLTF_TYPE_VEC2  )nofC = 2;
                if(accessor.type == TINYGLTF_TYPE_VEC3  )nofC = 3;
                if(accessor.type == TINYGLTF_TYPE_VEC4  )nofC = 4;
                if(stride == 0)stride = sizeof(float)*(uint32_t)nofC;
                m_mesh.vao->addAttrib(buffers.at(bufId),att,nofC,GL_FLOAT,stride,offset);
              }
            };
            if(std::string(attrib.first) == "POSITION"  )addAttrib(0);
            if(std::string(attrib.first) == "NORMAL"    )addAttrib(1);
            if(std::string(attrib.first) == "TEXCOORD_0")addAttrib(2);
          }
        }
      }
    }
      
    Node loadNode(tinygltf::Node const&root,tinygltf::Model const&model){
      Node res;
      res.mesh = root.mesh;
      if(root.matrix.size() == 16){
        for(int i=0;i<16;++i)
          res.modelMatrix[i/4][i%4] = (float)root.matrix[i];
      }else{
        if(root.translation.size() == 3){
          auto*p = root.translation.data();
          res.modelMatrix = res.modelMatrix*glm::translate(glm::mat4(1.f),glm::vec3(p[0],p[1],p[2]));
        }
        if(root.rotation.size() == 4){
          auto*p = root.rotation.data();
          glm::quat q;
          q[0]=(float)p[0];
          q[1]=(float)p[1];
          q[2]=(float)p[2];
          q[3]=(float)p[3];
          res.modelMatrix = res.modelMatrix*glm::toMat4(q);
        }
        if(root.scale.size() == 3){
          auto*p = root.scale.data();
          res.modelMatrix = res.modelMatrix*glm::scale(glm::mat4(1.f),glm::vec3(p[0],p[1],p[2]));
        }
      }
      for(auto c:root.children)
        res.children.emplace_back(loadNode(model.nodes.at(c),model));
      return res;
    }

    void drawNode(Node const&node,ge::gl::Program*prg,glm::mat4 const&modelMatrix){
      if(node.mesh>=0){
        auto const&mesh = meshes.at(node.mesh);


        if(mesh.diffuseTexture != -1){
          prg->set1i("useTexture",1);
          glBindTextureUnit(0,textures.at(mesh.diffuseTexture)->getId());
        }else{
          prg->set1i("useTexture",0);
          prg->set4fv("diffuseColor",glm::value_ptr(mesh.diffuseColor));
        }

        prg->setMatrix4fv("model",glm::value_ptr(modelMatrix*node.modelMatrix));

        mesh.vao->bind();
        if(mesh.hasIndices)
          glDrawElements(GL_TRIANGLES,mesh.nofIndices,mesh.indexType,(GLvoid*)mesh.indexOffset);
        else
          glDrawArrays(GL_TRIANGLES,0,mesh.nofIndices);

      }

      for(auto const&n:node.children){
        drawNode(n,prg,modelMatrix*node.modelMatrix);
      }

    }
    void draw(glm::mat4 const&proj,glm::mat4 const&view,ge::gl::Program*prg){
      prg->use();
      prg->setMatrix4fv("proj",glm::value_ptr(proj));
      prg->setMatrix4fv("view",glm::value_ptr(view));
    
      for(auto const&root:roots)
        drawNode(root,prg,glm::mat4(1.f));
    }
  
    std::vector<Mesh                            >meshes  ;
    std::vector<Node                            >roots   ;
    std::vector<std::shared_ptr<ge::gl::Texture>>textures;
    std::vector<std::shared_ptr<ge::gl::Buffer >>buffers ;
};

std::string const source = R".(

#ifdef VERTEX_SHADER
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform mat4 model = mat4(1.f);

layout(location = 0)in vec3 position;
layout(location = 1)in vec3 normal  ;
layout(location = 2)in vec2 texCoord;

out vec2 vCoord;
out vec3 vNormal;
out vec3 vPosition;
out vec3 vCamPosition;
void main(){
  vCoord  = texCoord;
  vNormal = normal  ;
  vPosition = vec3(model*vec4(position,1.f));
  vCamPosition = vec3(inverse(view)*vec4(0,0,0,1));
  gl_Position = proj*view*model*vec4(position,1.f);
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vPosition;
in vec2 vCoord;
in vec3 vNormal;
in vec3 vCamPosition;

uniform vec3  lightPosition = vec3(30,30,30)   ;
uniform vec3  lightColor    = vec3(1,1,1)      ;
uniform vec3  lightAmbient  = vec3(0.3,0.1,0.0);
uniform float shininess     = 60.f             ;


uniform sampler2D diffuseTexture;
uniform vec4      diffuseColor = vec4(1.f);
uniform int       useTexture   = 0;

layout(location=0)out vec4 fColor;
void main(){
  vec3 diffuseColor = vec3(0.3);
  fColor = vec4(vNormal,1);

  if(useTexture == 1)
    diffuseColor = texture(diffuseTexture,vCoord).rgb;
  else
    diffuseColor = diffuseColor.rgb;


  vec3 finalColor = phongLighting(
      vPosition          ,
      normalize(vNormal) ,
      lightPosition      ,
      vCamPosition       ,
      lightColor         ,
      lightAmbient       ,
      diffuseColor       ,
      shininess          ,
      0.f                );

    fColor = vec4(finalColor,1.f);

}
#endif
).";

std::string const modelShadowMappingVS = R".(
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform mat4 model = mat4(1.f);

uniform mat4 lightView = mat4(1.f);
uniform mat4 lightProj = mat4(1.f);
uniform mat4 lightBias = mat4(1.f);

layout(location = 0)in vec3 position;
layout(location = 1)in vec3 normal  ;
layout(location = 2)in vec2 texCoord;

out vec4 vShadowPos;
out vec3 vPosition;
out vec3 vNormal;
out vec2 vCoord;
out vec3 vCamPosition;

void main(){
  vec4 pos = vec4(position,1.f);

  vPosition    = vec3(pos);
  vNormal      = normal;
  vCoord       = texCoord;
  vCamPosition = vec3(inverse(view)*vec4(0,0,0,1));

  gl_Position  =                proj*     view*model*pos;
  vShadowPos   = lightBias*lightProj*lightView*model*pos;

}

).";


glm::mat4 getLightView      (vars::Vars&vars){
  auto&lightPosition = *vars.get<glm::vec3>("method.lightPosition");
  return glm::lookAt(lightPosition,glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));
}

glm::mat4 getLightProj(vars::Vars&vars){
  auto fovy = vars.addOrGetFloat("method.shadowFovy",0.2f);
  return glm::perspective(fovy,1.f,0.1f,5000.f);
}

void setShadowMappingUniforms(vars::Vars&vars,std::string const&prgName){
  auto prg = vars.get<Program>(prgName);

  auto lightView = getLightView(vars);
  auto lightProj = getLightProj(vars);
  auto bias      = glm::mat4(1.f);
  bias[0][0] = bias[1][1] = bias[2][2] = bias[3][0] = bias[3][1] = bias[3][2] = 0.5;

  prg
    ->setMatrix4fv("lightView",glm::value_ptr(lightView))
    ->setMatrix4fv("lightProj",glm::value_ptr(lightProj))
    ->setMatrix4fv("lightBias",glm::value_ptr(bias     ))
    ->set3fv      ("lightPosition",glm::value_ptr(*vars.get<glm::vec3>("method.lightPosition")));

}

void renderShadowMap(vars::Vars&vars){
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
  glClear(GL_DEPTH_BUFFER_BIT);
  *vars.addOrGet<glm::mat4>("method.scene.view") = getLightView(vars);
  *vars.addOrGet<glm::mat4>("method.scene.proj") = getLightProj(vars);

  auto res = vars.getUint32("method.shadowMapResolution");
  glViewport(0,0,res,res);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(2.5f,10.f);

  auto proj = vars.get<basicCamera::PerspectiveCamera>("method.proj")->getProjection();
  auto view = vars.get<basicCamera::OrbitCamera      >("method.view")->getView      ();

  auto prg = vars.get<ge::gl::Program    >("method.prg");
  auto mdl = vars.get<GModel>("method.model");
  mdl->draw(proj,view,prg);

  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");
  glViewport(0,0,width,height);

  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void onInit(vars::Vars&vars){
  vars.addUint32("method.shadowMapResolution",1024);
  model::setUpCamera(vars);
  vars.get<basicCamera::OrbitCamera>("method.view")->setDistance(10.f);
  vars.add<glm::vec3>("method.lightPosition",glm::vec3(30.f));

  vars.reCreate<ge::gl::Program>("method.prg",
      std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER    ,"#version 460\n#define   VERTEX_SHADER\n"  +source),
      std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER  ,"#version 460\n#define FRAGMENT_SHADER\n"  +phong::phongLightingShader+source)
      );

  auto mdl = vars.reCreate<GModel>("method.model");
  mdl->load(std::string(CMAKE_ROOT_DIR)+"/resources/models/konfucius/scene.gltf");
  //mdl->load(std::string(CMAKE_ROOT_DIR)+"/resources/models/nyra/scene.gltf");
  //mdl->load(std::string(CMAKE_ROOT_DIR)+"/resources/models/china.glb");
  //mdl->load(std::string(CMAKE_ROOT_DIR)+"/resources/models/triss/scene.gltf");


  glCreateTextures(GL_TEXTURE_2D,1,&shadowMap);
  glTextureStorage2D(shadowMap,1,GL_DEPTH24_STENCIL8,1024,1024);
  glTextureParameteri(shadowMap,GL_TEXTURE_COMPARE_FUNC,GL_LESS);
  glTextureParameteri(shadowMap,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_R_TO_TEXTURE);

  glCreateFramebuffers(1,&fbo);
  glNamedFramebufferTexture(fbo,GL_DEPTH_ATTACHMENT ,shadowMap,0);
  if(glCheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "framebuffer incomplete" << std::endl;

  glClearColor(0.1,0.1,0.1,1);
  glEnable(GL_DEPTH_TEST);
}

void initShadowedModel(vars::Vars&vars){
  FUNCTION_PROLOGUE("method");
  vars.reCreate<Program>("method.shadow_prg",
      std::make_shared<Shader>(GL_VERTEX_SHADER  ,"#version 460\n",modelShadowMappingVS),
      std::make_shared<Shader>(GL_FRAGMENT_SHADER,"#version 460\n",phong::phongLightingShader+shadowMappingFS)
      );
}

void drawShadowedModel(vars::Vars&vars){
  initShadowedModel(vars);

  auto proj = vars.get<basicCamera::PerspectiveCamera>("method.proj")->getProjection();
  auto view = vars.get<basicCamera::OrbitCamera      >("method.view")->getView      ();


  auto prg = vars.get<ge::gl::Program    >("method.shadow_prg");
  auto mdl = vars.get<GModel>("method.model");
  setShadowMappingUniforms(vars,"method.shadow_prg");
  mdl->draw(proj,view,prg);

}

void drawShadowedScene(vars::Vars&vars){
  glClearColor(0.2,0.2,0.2,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  *vars.addOrGet<glm::mat4>("method.scene.view") = vars.getReinterpret<basicCamera::CameraTransform >("method.view")->getView      ();
  *vars.addOrGet<glm::mat4>("method.scene.proj") = vars.getReinterpret<basicCamera::CameraProjection>("method.proj")->getProjection();
  glBindTextureUnit(0,shadowMap);
  drawShadowedModel(vars);
}


void onDraw(vars::Vars&vars){
  model::computeProjectionMatrix(vars);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  renderShadowMap(vars);

  drawShadowedScene(vars);

}

void onQuit(vars::Vars&vars){
  glDeleteTextures(1,&shadowMap);
  glDeleteFramebuffers(1,&fbo);
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  clbs.onMouseMotion = model      ::onMouseMotion;
  MethodRegister::get().manager.registerMethod("pgr03.shadowedModel",clbs);
};

}

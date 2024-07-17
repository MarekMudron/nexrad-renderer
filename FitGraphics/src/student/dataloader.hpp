#include <Radx/RadxFile.hh>
#include <Radx/NexradRadxFile.hh>
#include <Radx/RadxVol.hh>
#include <Radx/RadxRay.hh>
#include <algorithm>
using namespace ge::gl;

enum FieldChoice
{
  REF,
  VEL
};

struct Measurement
{
  float az;
  float elev;
  float dist;
  float value;
};

struct Ray
{
  float az;
  float elev;
  float radMin;
  float radMax;
  int row;
  float x;
  float y;
};

class DataLoader
{

  RadxVol vol;
  FieldChoice currentChoice = FieldChoice::REF;
  int N;
  int _numRays;

public:

  void loadVol(string path)
  {
    NexradRadxFile file;

    if (file.readFromPath(path, vol) != 0)
    {
      std::cerr << "Error reading NEXRAD Level 2 file." << std::endl;
      return;
    }
  }

  vector<Measurement> getMeasurement(FieldChoice choice)
  {
    string field;
    if (choice == VEL)
    {
      field = "VEL";
    }
    else
    {
      field = "REF";
    }

    auto rays = vol.getRays();
    vector<Measurement> measurements;
    for_each(rays.begin(), rays.end(), [&](RadxRay *ray)
             { 
      double az = ray->getAzimuthDeg();
      double elev = ray->getElevationDeg();
      const RadxField *velField = ray->getField(field);
      auto scale = velField->getScale();
      auto offset = velField->getOffset();
  
      auto velData = velField->getDataSi08();
      auto nPoints = velField->getNPoints();
      std::vector<Radx::si08> velDataVec(velData, velData + nPoints);

      auto missingTag = velField->getMissingSi08();
      double startkm = velField->getStartRangeKm();
      double spacingkm = velField->getGateSpacingKm();
      
      double distkm = startkm;
      for_each(velDataVec.begin(),  velDataVec.end(),[&](Radx::si08 val) {
        if(val != missingTag) {
          Measurement measObj = {az, elev, distkm, offset + (scale * static_cast<float>(val))};
          measurements.push_back(measObj);
        }
        distkm += spacingkm;
      }); });
    N = measurements.size();
    return measurements;
  }

  vector<Ray> getRays()
  {
    auto rays = vol.getRays();
    vector<Ray> raysOut;
    int row = 0;
    double lastFixedAngle = 0;
    for_each(rays.begin(), rays.end(), [&](RadxRay *ray)
             { 
      double az = ray->getAzimuthDeg();
      double elev = ray->getElevationDeg();
      double startkm = ray->getStartRangeKm();
      double spacingkm = ray->getGateSpacingKm();

      double fixedAngle = ray->getFixedAngleDeg();
      if(fixedAngle != lastFixedAngle) {
        lastFixedAngle = fixedAngle;
        row++;
      }
      double distkm = startkm;
      float MAX_DIST_KM = (2.125 + 0.25 *1832);
      Ray rayObj = {az, elev, startkm, MAX_DIST_KM, row, 0.f,0.f};
      raysOut.push_back(rayObj); });
    _numRays = raysOut.size();
    return raysOut;
  }

  int numRays()
  {
    return _numRays;
  }

  int numMeasurements()
  {
    return N;
  }

  bool needReload(FieldChoice choice)
  {
    if (choice != currentChoice)
    {
      currentChoice = choice;
      return true;
    }
    return false;
  }
};
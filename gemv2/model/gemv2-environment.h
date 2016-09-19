/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Karsten Roscher
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef GEMV2_ENVIRONMENT_H
#define GEMV2_ENVIRONMENT_H

/*
 * Note: iostream is required due to a boost bug in 1.61
 *
 * rtree.hpp includes some other header files, one of them is
 * 'boost/geometry/algorithms/detail/overlay/handle_colocations.hpp' which
 * uses std::cout for some strange reason. Most probably, somebody forgot to
 * add the defines for debug output.
 *
 * There is already an open bug report:
 * https://svn.boost.org/trac/boost/ticket/12443
 */
#include <iostream>
#include <vector>

#include <boost/geometry/index/rtree.hpp>

#include <ns3/ptr.h>
#include <ns3/simple-ref-count.h>

#include <ns3/gemv2-building.h>
#include <ns3/gemv2-foliage.h>
#include <ns3/gemv2-vehicle.h>

namespace ns3 {
namespace gemv2 {

/*!
 * @brief Class to manage the GEMV^2 environment.
 *
 * This will bundle all actors (vehicles, RSUs) and objects
 * (buildings, foliage) in one place.
 */
class Environment : public SimpleRefCount<Environment>
{
public:

  /*
   * Some typedefs
   */

  //! List of buildings
  typedef std::vector<Ptr<Building>> BuildingList;

  //! List of foliage objects
  typedef std::vector<Ptr<Foliage>> FoliageList;

  //! List of vehicles
  typedef std::vector<Ptr<Vehicle>> VehicleList;

  //! Collection of objects
  struct ObjectCollection
  {
    BuildingList buildings;
    FoliageList foliage;
    VehicleList vehicles;
  };

  /*
   * Class members
   */

  /*!
   * @brief Create empty environment.
   */
  Environment();

  /*!
   * @brief Get global instance of the environment.
   * @return Global instance of the environment
   */
  static Ptr<Environment>
  GetGlobal ();

  /*!
   * @brief Add a building to the environment.
   * @param building	Building to add, must not be null
   */
  void
  AddBuilding (Ptr<Building> building);

  /*!
   * @brief Add buildings to the environment.
   * @param buildings	Buildings to add, must not be null
   */
  void
  AddBuildings (const BuildingList& buildings);

  /*!
   * @brief Add a foliage object to the environment.
   * @param foliage	Foliage to add, must not be null
   */
  void
  AddFoliage (Ptr<Foliage> foliage);

  /*!
   * @brief Test if line intersects with any buildings
   * @param line 	Line to test
   * @return True if line intersects at least with one building
   */
  bool
  IntersectsBuildings (const LineSegment2d& line) const;

  /*!
   * @brief Test if line intersects with any foliage
   * @param line 	Line to test
   * @return True if line intersects at least with one foliage object
   */
  bool
  IntersectsFoliage (const LineSegment2d& line) const;

  /*!
   * @brief Calculate intersection of a line segment with buildings.
   * @param line		Line to calculate the intersections for
   * @param outBuildings 	Buildings intersecting with @a line
   */
  void
  Intersect (const LineSegment2d& line, BuildingList& outBuildings);

  /*!
   * @brief Calculate intersection of a line segment with foliage objects.
   * @param line		Line to calculate the intersections for
   * @param outFoliage	 	Foliage objects intersecting with @a line
   */
  void
  Intersect (const LineSegment2d& line, FoliageList& outFoliage);

  /*!
   * @brief Calculate intersection of a line segment with vehicles.
   * @param line		Line to calculate the intersections for
   * @param outVehicles	 	Vehicles intersecting with @a line
   */
  void
  Intersect (const LineSegment2d& line, VehicleList& outVehicles);

  /*!
   * @brief Find all buildings in ellipse.
   * @param p1			First focal point of the ellipse
   * @param p2			Second focal point of the ellipse
   * @param range		Length of the major diameter
   * @param outBuildings	Buildings where the sum of distance to @a p1 and
   * 				@a p2 is less than @a range.
   */
  void
  FindInEllipse (const Point2d& p1, const Point2d& p2, double range,
		 BuildingList& outBuildings);

  /*!
   * @brief Find all foliage in ellipse.
   * @param p1			First focal point of the ellipse
   * @param p2			Second focal point of the ellipse
   * @param range		Length of the major diameter
   * @param outFoliage		Foliage where the sum of distance to @a p1 and
   * 				@a p2 is less than @a range.
   */
  void
  FindInEllipse (const Point2d& p1, const Point2d& p2, double range,
		 FoliageList& outFoliage);

  /*!
   * @brief Find all vehicles in ellipse.
   * @param p1			First focal point of the ellipse
   * @param p2			Second focal point of the ellipse
   * @param range		Length of the major diameter
   * @param outVehicles 	Vehicles where the sum of distance to @a p1 and
   * 				@a p2 is less than @a range.
   */
  void
  FindInEllipse (const Point2d& p1, const Point2d& p2, double range,
		 VehicleList& outVehicles);

  /*!
   * @brief Find all objects in ellipse.
   * @param p1			First focal point
   * @param p2			Second focal point
   * @param range		Maximum combined distance to @a p1 and @a p2
   * @param outObjects		Found objects in ellipse are added here
   */
  void
  FindAllInEllipse (const Point2d& p1, const Point2d& p2, double range,
		    ObjectCollection& outObjects);

private:

  /*
   * Internal data stuctures
   */

  // small indexer for the trees to allow using pointers directly
  template <typename T>
  struct PtrIndex
  {
      typedef ns3::Ptr<T> V;
      typedef ns3::gemv2::Box2d const& result_type;
      result_type operator()(V const& v) const { return v->GetBoundingBox (); }
  };

  /*!
   * @brief Type of a range tree for buildings
   *
   * The tree will only be build on startup. Thus, we use the more
   * expensive r-star algorithm here since it provides better performance
   * for queries.
   */
  typedef boost::geometry::index::rtree<
      Ptr<Building>, boost::geometry::index::quadratic<16>, PtrIndex<Building>
  > BuildingTree;

  //! The range tree containing all buildings
  BuildingTree m_buildings;

  /*!
   * @brief Type of a range tree for foliage
   *
   * The tree will only be build on startup. Thus, we use the more
   * expensive r-star algorithm here since it provides better performance
   * for queries.
   */
  typedef boost::geometry::index::rtree<
      Ptr<Foliage>, boost::geometry::index::quadratic<16>, PtrIndex<Foliage>
  > FoliageTree;

  //! The range tree containing all foliage objects
  FoliageTree m_foliage;

  //! List of all vehicles
  std::vector<Ptr<Vehicle>> m_vehicles;
};

}  // namespace gemv2
}  // namespace ns3

#endif /* GEMV2_ENVIRONMENT_H */

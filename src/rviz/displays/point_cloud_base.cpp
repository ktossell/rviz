/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "point_cloud_base.h"
#include "common.h"
#include "ros_topic_property.h"
#include "properties/property.h"
#include "properties/property_manager.h"

#include "ros/node.h"
#include <ros/time.h>
#include "ogre_tools/point_cloud.h"

#include <tf/transform_listener.h>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

namespace rviz
{

PointCloudBase::CloudInfo::CloudInfo()
: time_(0.0f)
, num_points_(0)
{}

PointCloudBase::CloudInfo::~CloudInfo()
{
}

PointCloudBase::PointCloudBase( const std::string& name, VisualizationManager* manager )
: Display( name, manager )
, new_cloud_(false)
, min_color_( 0.0f, 0.0f, 0.0f )
, max_color_( 1.0f, 1.0f, 1.0f )
, min_intensity_(0.0f)
, max_intensity_(4096.0f)
, auto_compute_intensity_bounds_(true)
, intensity_bounds_changed_(false)
, style_( Billboards )
, channel_color_idx_( -1 )
, billboard_size_( 0.01 )
, point_decay_time_(0.0f)
, billboard_size_property_( NULL )
, alpha_property_(NULL)
, min_color_property_( NULL )
, max_color_property_( NULL )
, auto_compute_intensity_bounds_property_( NULL )
, min_intensity_property_( NULL )
, max_intensity_property_( NULL )
, style_property_( NULL )
, channel_property_( NULL )
, decay_time_property_( NULL )
{
  cloud_ = new ogre_tools::PointCloud( scene_manager_, NULL );

  setStyle( style_ );
  setBillboardSize( billboard_size_ );
  setChannelColorIndex ( channel_color_idx_ );
  setAlpha(1.0f);
}

PointCloudBase::~PointCloudBase()
{
  delete cloud_;
}

void PointCloudBase::setAlpha( float alpha )
{
  alpha_ = alpha;

  cloud_->setAlpha(alpha_);

  if ( alpha_property_ )
  {
    alpha_property_->changed();
  }
}

void PointCloudBase::setMaxColor( const Color& color )
{
  max_color_ = color;

  if ( max_color_property_ )
  {
    max_color_property_->changed();
  }

  causeRender();
}

void PointCloudBase::setMinColor( const Color& color )
{
  min_color_ = color;

  if ( min_color_property_ )
  {
    min_color_property_->changed();
  }

  causeRender();
}

void PointCloudBase::setMinIntensity( float val )
{
  min_intensity_ = val;
  if (min_intensity_ > max_intensity_)
  {
    min_intensity_ = max_intensity_;
  }

  if (min_intensity_property_)
  {
    min_intensity_property_->changed();
  }

  causeRender();
}

void PointCloudBase::setMaxIntensity( float val )
{
  max_intensity_ = val;
  if (max_intensity_ < min_intensity_)
  {
    max_intensity_ = min_intensity_;
  }

  if (max_intensity_property_)
  {
    max_intensity_property_->changed();
  }

  causeRender();
}

void PointCloudBase::setDecayTime( float time )
{
  point_decay_time_ = time;

  if ( decay_time_property_ )
  {
    decay_time_property_->changed();
  }

  causeRender();
}

void PointCloudBase::setAutoComputeIntensityBounds(bool compute)
{
  auto_compute_intensity_bounds_ = compute;

  if (auto_compute_intensity_bounds_property_)
  {
    auto_compute_intensity_bounds_property_->changed();
  }

  causeRender();
}

void PointCloudBase::setStyle( int style )
{
  ROS_ASSERT( style < StyleCount );

  {
    RenderAutoLock renderLock( this );

    style_ = style;

    cloud_->setUsePoints(style == Points);
  }

  if ( style_property_ )
  {
    style_property_->changed();
  }

  causeRender();
}

/** \brief Set the channel color index. Called through the \a channel_property_ callback.
  * \param channel_color_idx the index of the channel to be rendered
  */
void
  PointCloudBase::setChannelColorIndex (int channel_color_idx)
{
  ROS_ASSERT (channel_color_idx < ChannelRenderCount);

  channel_color_idx_ = channel_color_idx;
}

void PointCloudBase::setBillboardSize( float size )
{
  {
    RenderAutoLock renderLock( this );

    billboard_size_ = size;

    cloud_->setBillboardDimensions( size, size );
  }

  if ( billboard_size_property_ )
  {
    billboard_size_property_->changed();
  }

  causeRender();
}

void PointCloudBase::onEnable()
{
}

void PointCloudBase::onDisable()
{
  clouds_.clear();
  cloud_->clear();
}

void PointCloudBase::update(float dt)
{
  if (intensity_bounds_changed_)
  {
    setMinIntensity(min_intensity_);
    setMaxIntensity(max_intensity_);
    intensity_bounds_changed_ = false;
  }

  {
    boost::mutex::scoped_lock lock(clouds_mutex_);

    D_CloudInfo::iterator cloud_it = clouds_.begin();
    D_CloudInfo::iterator cloud_end = clouds_.end();
    for (;cloud_it != cloud_end; ++cloud_it)
    {
      const CloudInfoPtr& info = *cloud_it;

      info->time_ += dt;
    }

    if (point_decay_time_ > 0.0f)
    {
      bool removed = false;
      while (!clouds_.empty() && clouds_.front()->time_ > point_decay_time_)
      {
        cloud_->popPoints(clouds_.front()->num_points_);
        clouds_.pop_front();
        removed = true;
      }

      if (removed)
      {
        causeRender();
      }
    }

    if (new_cloud_ && !clouds_.empty())
    {
      const boost::shared_ptr<robot_msgs::PointCloud>& cloud = clouds_.front()->message_;

      // Get the channels that we could potentially render
      int channel_color_idx = getChannelColorIndex ();
      channel_property_->clear ();
      typedef std::vector<robot_msgs::ChannelFloat32> V_Chan;
      V_Chan::iterator chan_it = cloud->chan.begin();
      V_Chan::iterator chan_end = cloud->chan.end();
      uint32_t index = 0;
      for ( ; chan_it != chan_end; ++chan_it, ++index )
      {
        robot_msgs::ChannelFloat32& chan = *chan_it;
        if (chan.name == "intensity" || chan.name == "intensities")
        {
          if (channel_color_idx == -1)
          {
            channel_color_idx = Intensity;
          }

          channel_property_->addOption ("Intensity", Intensity);
        }
        else if (chan.name == "rgb" || chan.name == "r")
        {
          if (channel_color_idx == -1)
          {
            channel_color_idx = ColorRGBSpace;
          }

          channel_property_->addOption ("Color (RGB)", ColorRGBSpace);
        }
        else if (chan.name == "nx")
        {
          if (channel_color_idx == -1)
          {
            channel_color_idx = NormalSphere;
          }

          channel_property_->addOption ("Normal Sphere", NormalSphere);
        }
        else if (chan.name == "curvature" || chan.name == "curvatures")
        {
          if (channel_color_idx == -1)
          {
            channel_color_idx = Curvature;
          }

          channel_property_->addOption ("Curvature", Curvature);
        }
      }

      if ( channel_property_ )
      {
        channel_property_->changed();
      }

      setChannelColorIndex (channel_color_idx);
    }

    new_cloud_ = false;
  }
}

void transformIntensity( float val, ogre_tools::PointCloud::Point& point, const Color& min_color, float min_intensity, float max_intensity, float diff_intensity )
{
  float normalized_intensity = diff_intensity > 0.0f ? ( val - min_intensity ) / diff_intensity : 1.0f;
  normalized_intensity = std::min(1.0f, std::max(0.0f, normalized_intensity));
  point.r_ = point.r_*normalized_intensity + min_color.r_*(1.0f - normalized_intensity);
  point.g_ = point.g_*normalized_intensity + min_color.g_*(1.0f - normalized_intensity);
  point.b_ = point.b_*normalized_intensity + min_color.b_*(1.0f - normalized_intensity);
}

void transformRGB( float val, ogre_tools::PointCloud::Point& point, const Color&, float, float, float )
{
  int rgb = *reinterpret_cast<int*>(&val);
  point.r_ = ((rgb >> 16) & 0xff) / 255.0f;
  point.g_ = ((rgb >> 8) & 0xff) / 255.0f;
  point.b_ = (rgb & 0xff) / 255.0f;
}

void transformR( float val, ogre_tools::PointCloud::Point& point, const Color&, float, float, float )
{
  point.r_ = val;
}

void transformG( float val, ogre_tools::PointCloud::Point& point, const Color&, float, float, float )
{
  point.g_ = val;
}

void transformB( float val, ogre_tools::PointCloud::Point& point, const Color&, float, float, float )
{
  point.b_ = val;
}

void PointCloudBase::processMessage(const boost::shared_ptr<robot_msgs::PointCloud>& cloud)
{
  CloudInfoPtr info(new CloudInfo());
  info->message_ = cloud;
  info->time_ = 0;

  transformCloud(info);

  {
    boost::mutex::scoped_lock lock(clouds_mutex_);

    if (point_decay_time_ == 0.0f)
    {
      clouds_.clear();
    }

    clouds_.push_back(info);

    new_cloud_ = true;
  }
}

void PointCloudBase::transformCloud(const CloudInfoPtr& info)
{
  const boost::shared_ptr<robot_msgs::PointCloud>& cloud = info->message_;

  std::string frame_id = cloud->header.frame_id;
  if ( frame_id.empty() )
  {
    frame_id = fixed_frame_;
  }

  try
  {
    tf_->transformPointCloud( fixed_frame_, *cloud, *cloud );
  }
  catch(tf::TransformException& e)
  {
    ROS_ERROR( "Error transforming point cloud '%s' from frame '%s' to frame '%s'\n", name_.c_str(), frame_id.c_str(), fixed_frame_.c_str() );
  }

  typedef std::vector<robot_msgs::ChannelFloat32> V_Chan;
  typedef std::vector<bool> V_bool;

  V_bool valid_channels(cloud->chan.size());
  uint32_t point_count = cloud->get_pts_size();
  V_Chan::iterator chan_it = cloud->chan.begin();
  V_Chan::iterator chan_end = cloud->chan.end();
  uint32_t index = 0;

  info->num_points_ = point_count;

  bool use_normals_as_coordinates = false;

  for ( ; chan_it != chan_end; ++chan_it, ++index )
  {
    robot_msgs::ChannelFloat32& chan = *chan_it;
    uint32_t val_count = chan.vals.size();
    bool channel_size_correct = val_count == point_count;
    ROS_ERROR_COND(!channel_size_correct, "Point cloud '%s' has channel with fewer values than points (%d values, %d points)", name_.c_str(), val_count, point_count);

    valid_channels[index] = channel_size_correct;

    // Check for intensities
    if ( auto_compute_intensity_bounds_ && channel_size_correct && ( chan.name == "intensity" || chan.name == "intensities" ) &&
         channel_color_idx_ == Intensity )
    {
      min_intensity_ = 999999.0f;
      max_intensity_ = -999999.0f;
      for(uint32_t i = 0; i < point_count; i++)
      {
        float& intensity = chan.vals[i];
        // arbitrarily cap to 4096 for now
        intensity = std::min( intensity, 4096.0f );
        min_intensity_ = std::min( min_intensity_, intensity );
        max_intensity_ = std::max( max_intensity_, intensity );
      }

      intensity_bounds_changed_ = true;
    }

    // Check for curvatures
    else if ( auto_compute_intensity_bounds_ && channel_size_correct && ( chan.name == "curvature" || chan.name == "curvatures" ) &&
              channel_color_idx_ == Curvature )
    {
      min_intensity_ = 999999.0f;
      max_intensity_ = -999999.0f;
      for(uint32_t i = 0; i < point_count; i++)
      {
        float& intensity = chan.vals[i];
        // arbitrarily cap to 4096 for now
        intensity = std::min( intensity, 4096.0f );
        min_intensity_ = std::min( min_intensity_, intensity );
        max_intensity_ = std::max( max_intensity_, intensity );
      }

      intensity_bounds_changed_ = true;
    }
    else if ( chan.name == "nx" && channel_color_idx_ == NormalSphere )
      use_normals_as_coordinates = true;
  }

  // Look for point normals
  int nx_idx = getROSCloudChannelIndex (cloud, std::string ("nx"));
  int ny_idx = getROSCloudChannelIndex (cloud, std::string ("ny"));
  int nz_idx = getROSCloudChannelIndex (cloud, std::string ("nz"));
  if ( use_normals_as_coordinates && (ny_idx == -1 || nz_idx == -1) )
  {
    ROS_WARN ("Normal information requested via 'nx', but 'ny' and 'nz' channels are not present!");
    use_normals_as_coordinates = false;
  }

  float diff_intensity = max_intensity_ - min_intensity_;

  typedef std::vector< ogre_tools::PointCloud::Point > V_Point;
  V_Point points;
  points.resize( point_count );
  for(uint32_t i = 0; i < point_count; i++)
  {
    ogre_tools::PointCloud::Point& current_point = points[ i ];

    if (use_normals_as_coordinates && nx_idx != -1 && ny_idx != -1 && nz_idx != -1)
    {
      current_point.x_ = cloud->chan[nx_idx].vals[i];
      current_point.y_ = cloud->chan[ny_idx].vals[i];
      current_point.z_ = cloud->chan[nz_idx].vals[i];
    }
    else          // Use normal 3D x-y-z coordinates
    {
      current_point.x_ = cloud->pts[i].x;
      current_point.y_ = cloud->pts[i].y;
      current_point.z_ = cloud->pts[i].z;
    }

    Ogre::Vector3 position( current_point.x_, current_point.y_, current_point.z_ );
    robotToOgre( position );
    current_point.x_ = position.x;
    current_point.y_ = position.y;
    current_point.z_ = position.z;

    current_point.r_ = max_color_.r_;
    current_point.g_ = max_color_.g_;
    current_point.b_ = max_color_.b_;
  }

  index = 0;
  enum ChannelType
  {
    CT_INTENSITY,
    CT_RGB,
    CT_R,
    CT_G,
    CT_B,

    CT_COUNT
  };
  ChannelType type = CT_INTENSITY;
  typedef void (*TransformFunc)(float, ogre_tools::PointCloud::Point&, const Color&, float, float, float);
  TransformFunc funcs[CT_COUNT] =
  {
    transformIntensity,
    transformRGB,
    transformR,
    transformG,
    transformB
  };

  chan_it = cloud->chan.begin();
  for ( ; chan_it != chan_end; ++chan_it, ++index )
  {
    if ( !valid_channels[index] )
    {
      continue;
    }

    robot_msgs::ChannelFloat32& chan = *chan_it;

    if ( chan.name == "intensity" || chan.name == "intensities" || chan.name == "curvatures" || chan.name == "curvature" )
    {
      type = CT_INTENSITY;
    }
    else if ( chan.name == "rgb" )
    {
      type = CT_RGB;
    }
    else if ( chan.name == "r" )
    {
      type = CT_R;
    }
    else if ( chan.name == "g" )
    {
      type = CT_G;
    }
    else if ( chan.name == "b" )
    {
      type = CT_B;
    }
    else
    {
      continue;
    }

    // Color all points
    if ( ( channel_color_idx_ == Intensity && (chan.name == "intensity" || chan.name == "intensities") ) ||
           ( channel_color_idx_ == Curvature && (chan.name == "curvature" || chan.name == "curvatures") ) ||
           ( channel_color_idx_ == ColorRGBSpace && (chan.name == "rgb" || chan.name == "r" || chan.name == "g" || chan.name == "b") )
         )
    {
      for (uint32_t i = 0; i < point_count; i++)
      {
        ogre_tools::PointCloud::Point& current_point = points[ i ];

        funcs[type]( chan.vals[i], current_point, min_color_, min_intensity_, max_intensity_, diff_intensity );
      } // for point_count
    }
  }

  {
    RenderAutoLock renderLock( this );

    if (point_decay_time_ == 0.0f)
    {
      cloud_->clear();
    }

    if ( !points.empty() )
    {
      cloud_->addPoints( &points.front(), points.size() );
    }
  }

  causeRender();

}

void PointCloudBase::addMessage(const boost::shared_ptr<robot_msgs::PointCloud>& cloud)
{
  processMessage(cloud);
}

void PointCloudBase::fixedFrameChanged()
{
  reset();
}

void PointCloudBase::createProperties()
{
  style_property_ = property_manager_->createProperty<EnumProperty>( "Style", property_prefix_, boost::bind( &PointCloudBase::getStyle, this ),
                                                                     boost::bind( &PointCloudBase::setStyle, this, _1 ), parent_category_, this );
  style_property_->addOption( "Billboards", Billboards );
  style_property_->addOption( "Points", Points );

  channel_property_ = property_manager_->createProperty<EnumProperty>( "Channel", property_prefix_, boost::bind( &PointCloudBase::getChannelColorIndex, this ),
                                                                     boost::bind( &PointCloudBase::setChannelColorIndex, this, _1 ), parent_category_, this );

  alpha_property_ = property_manager_->createProperty<FloatProperty>( "Alpha", property_prefix_, boost::bind( &PointCloudBase::getAlpha, this ),
                                                                        boost::bind( &PointCloudBase::setAlpha, this, _1 ), parent_category_, this );

  min_color_property_ = property_manager_->createProperty<ColorProperty>( "Min Color", property_prefix_, boost::bind( &PointCloudBase::getMinColor, this ),
                                                                            boost::bind( &PointCloudBase::setMinColor, this, _1 ), parent_category_, this );
  max_color_property_ = property_manager_->createProperty<ColorProperty>( "Max Color", property_prefix_, boost::bind( &PointCloudBase::getMaxColor, this ),
                                                                        boost::bind( &PointCloudBase::setMaxColor, this, _1 ), parent_category_, this );
  // legacy "Color" support... convert it to max color
  max_color_property_->addLegacyName("Color");

  billboard_size_property_ = property_manager_->createProperty<FloatProperty>( "Billboard Size", property_prefix_, boost::bind( &PointCloudBase::getBillboardSize, this ),
                                                                                boost::bind( &PointCloudBase::setBillboardSize, this, _1 ), parent_category_, this );
  billboard_size_property_->setMin( 0.0001 );

  auto_compute_intensity_bounds_property_ = property_manager_->createProperty<BoolProperty>( "Autocompute Intensity Bounds", property_prefix_, boost::bind( &PointCloudBase::getAutoComputeIntensityBounds, this ),
                                                                            boost::bind( &PointCloudBase::setAutoComputeIntensityBounds, this, _1 ), parent_category_, this );
  min_intensity_property_ = property_manager_->createProperty<FloatProperty>( "Min Intensity", property_prefix_, boost::bind( &PointCloudBase::getMinIntensity, this ),
                                                                            boost::bind( &PointCloudBase::setMinIntensity, this, _1 ), parent_category_, this );
  max_intensity_property_ = property_manager_->createProperty<FloatProperty>( "Max Intensity", property_prefix_, boost::bind( &PointCloudBase::getMaxIntensity, this ),
                                                                          boost::bind( &PointCloudBase::setMaxIntensity, this, _1 ), parent_category_, this );

  decay_time_property_ = property_manager_->createProperty<FloatProperty>( "Decay Time", property_prefix_, boost::bind( &PointCloudBase::getDecayTime, this ),
                                                                           boost::bind( &PointCloudBase::setDecayTime, this, _1 ), parent_category_, this );
}

void PointCloudBase::reset()
{
  {
    RenderAutoLock renderLock( this );

    clouds_.clear();
    cloud_->clear();
  }
}

} // namespace rviz
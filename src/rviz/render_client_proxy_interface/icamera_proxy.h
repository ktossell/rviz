/*
 * Copyright (c) 2010, Willow Garage, Inc.
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

#ifndef RVIZ_RENDER_CLIENT_PROXY_INTERFACE_ICAMERA_PROXY_H
#define RVIZ_RENDER_CLIENT_PROXY_INTERFACE_ICAMERA_PROXY_H

#include "iproxy.h"

namespace rviz
{
class UUID;
class Vector3;
class Quaternion;

namespace render_client_proxy_interface
{

class ICameraProxy : public IProxy
{
public:
  virtual void setPosition(const UUID&, const Vector3&) = 0;
  virtual void setOrientation(const UUID&, const Quaternion&) = 0;
  virtual void lookAt(const UUID&, const Vector3&) = 0;
  virtual void move(const UUID&, const Vector3&) = 0;
  virtual void moveRelative(const UUID&, const Vector3&) = 0;
  virtual void rotate(const UUID&, const Quaternion&) = 0;
  virtual void setFOVY(const UUID&, float fovy) = 0;
  virtual void setAspectRatio(const UUID&, float aspect) = 0;
  virtual void setAutoAspectRatio(const UUID&, bool autoratio) = 0;
  virtual void setNearClipDistance(const UUID&, float dist) = 0;
  virtual void setFarClipDistance(const UUID&, float dist) = 0;
  virtual void setTransform(const UUID&, const Vector3& pos, const Quaternion& orient) = 0;
};

} // namespace render_client_proxy_interface
} // namespace rviz

#endif // RVIZ_RENDER_CLIENT_PROXY_INTERFACE_ICAMERA_PROXY_H

/** @file
  * @copyright Copyright (c) 2012 PROFACTOR GmbH. All rights reserved. 
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *
  *     * Redistributions of source code must retain the above copyright
  * notice, this list of conditions and the following disclaimer.
  *     * Redistributions in binary form must reproduce the above
  * copyright notice, this list of conditions and the following disclaimer
  * in the documentation and/or other materials provided with the
  * distribution.
  *     * Neither the name of Google Inc. nor the names of its
  * contributors may be used to endorse or promote products derived from
  * this software without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  * @authors christoph.kopf@profactor.at
  *          florian.eckerstorfer@profactor.at
  */

#include "frame_grabber.h"
#include "reme_resource_manager.h"

#include <reconstructmesdk/reme.h>

#include <QCoreApplication>
#include <qdebug.h>

#include <iostream>


namespace ReconstructMeGUI {
  frame_grabber::frame_grabber(std::shared_ptr<reme_resource_manager> initializer) : 
    _i(initializer)
  {
    _req_count[REME_IMAGE_AUX] = 0;
    _req_count[REME_IMAGE_DEPTH] = 0;
    _req_count[REME_IMAGE_VOLUME] = 0;

    qRegisterMetaType<reme_sensor_image_t>("reme_sensor_image_t");

    connect(_i.get(), SIGNAL(initializing_sdk()), SLOT(stop()), Qt::BlockingQueuedConnection);
    connect(_i.get(), SIGNAL(sdk_initialized(bool)), SLOT(start(bool)));
  }
    
  frame_grabber::~frame_grabber() {
  }

  bool frame_grabber::is_grabbing() {
    return _do_grab;
  }

  void frame_grabber::request(reme_image_t image)
  {
    _req_count[image] += 1;
  }

  void frame_grabber::release(reme_image_t image)
  {
    _req_count[image] = std::max<int>(0, _req_count[image] - 1);
  }

  void frame_grabber::start(bool initialization_success) {
    if (!initialization_success) return;

    // Image creation
    reme_image_create(_i->context(), &_rgb);
    reme_image_create(_i->context(), &_depth);
    reme_image_create(_i->context(), &_phong);

    // Grabbing utils
    _do_grab = true;
    bool success = true;

    while (_do_grab && success)
    {
      // Prepare image and depth data
      success = success && REME_SUCCESS(reme_sensor_grab(_i->context(), _i->sensor()));

      if (_req_count[REME_IMAGE_AUX] > 0 && _i->rgb_size()) {
        reme_sensor_prepare_image(_i->context(), _i->sensor(), REME_IMAGE_AUX);
        reme_sensor_get_image(_i->context(), _i->sensor(), REME_IMAGE_AUX, _rgb);
        emit frame(REME_IMAGE_AUX, _rgb);
      }

      if (_req_count[REME_IMAGE_DEPTH] > 0 && _i->depth_size()) {
        reme_sensor_prepare_image(_i->context(), _i->sensor(), REME_IMAGE_DEPTH);
        reme_sensor_get_image(_i->context(), _i->sensor(), REME_IMAGE_DEPTH, _depth);
        emit frame(REME_IMAGE_DEPTH, _depth);
      }

      if (_req_count[REME_IMAGE_VOLUME] > 0 && _i->depth_size()) {
        reme_sensor_prepare_image(_i->context(), _i->sensor(), REME_IMAGE_VOLUME);
        reme_sensor_get_image(_i->context(), _i->sensor(), REME_IMAGE_VOLUME, _phong);
        emit frame(REME_IMAGE_VOLUME, _phong);
      }      

      emit frames_updated();
      QCoreApplication::processEvents();
    }

    emit stopped_grabbing();
  }

  void frame_grabber::stop() {
    _do_grab = false;
  }
}
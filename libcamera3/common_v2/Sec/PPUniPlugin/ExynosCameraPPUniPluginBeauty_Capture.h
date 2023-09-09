/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*!
 * \file      ExynosCameraPPUniPluginBeauty_Capture.h
 * \brief     header file for ExynosCameraPPUniPluginBeauty_Capture
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2016/10/05
 *
 * <b>Revision History: </b>
 * - 2016/10/05 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_PP_UNIPLUGIN_VIDEO_BEAUTY_CAPTURE_H
#define EXYNOS_CAMERA_PP_UNIPLUGIN_VIDEO_BEAUTY_CAPTURE_H

#include "ExynosCameraDefine.h"

#include "ExynosCameraPPUniPlugin.h"

using namespace android;

enum beauty_capture_status {
    BEAUTY_CAPTURE_DEINIT              = 0,
    BEAUTY_CAPTURE_IDLE,
    BEAUTY_CAPTURE_RUN,
};

/*
 * Class ExynosCameraPPUniPluginBeauty_Capture
 */
class ExynosCameraPPUniPluginBeauty_Capture : public ExynosCameraPPUniPlugin
{
protected:
    ExynosCameraPPUniPluginBeauty_Capture()
    {
        m_init();
    }

    ExynosCameraPPUniPluginBeauty_Capture(
            int cameraId,
            ExynosCameraConfigurations *configurations,
            ExynosCameraParameters *parameters,
            int nodeNum) : ExynosCameraPPUniPlugin(cameraId, configurations, parameters, nodeNum)
    {
        strncpy(m_name, "ExynosCameraPPUniPluginBeauty_Capture",  EXYNOS_CAMERA_NAME_STR_SIZE - 1);

        m_init();
    }

    /* ExynosCameraPPUniPluginBeauty_Capture's constructor is protected
     * to prevent new without ExynosCameraPPFactory::newPP()
     */
    friend class ExynosCameraPPFactory;

public:
    virtual ~ExynosCameraPPUniPluginBeauty_Capture();
    status_t    start(void);
    status_t    stop(bool suspendFlag = false);

protected:
    virtual status_t m_draw(ExynosCameraImage *srcImage,
                                ExynosCameraImage *dstImage,
                                ExynosCameraParameters *params);

private:
    void                m_init(void);
    status_t            m_UniPluginInit(void);
    status_t            process(ExynosCameraImage *srcImage,
                                    ExynosCameraImage *dstImage,
                                    ExynosCameraParameters *params);

    int                             m_refCount;

    int                             m_beautyStatus;
    mutable Mutex                   m_uniPluginLock;
protected:
};

#endif //EXYNOS_CAMERA_PP_UNIPLUGIN_VIDEO_BEAUTY_CAPTURE_H
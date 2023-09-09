/*
**
** Copyright 2014, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraFrameFactoryPreview"
#include <cutils/log.h>

#include "ExynosCameraFrameFactoryPreview.h"

namespace android {

ExynosCameraFrameFactoryPreview::~ExynosCameraFrameFactoryPreview()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);
}

status_t ExynosCameraFrameFactoryPreview::create(bool active)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_setupConfig();

    int ret = 0;
    int32_t nodeNums[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++)
        nodeNums[i] = -1;

    m_pipes[INDEX(PIPE_FLITE)] = (ExynosCameraPipe*)new ExynosCameraPipeFlite(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_FLITE)]);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeId(PIPE_FLITE);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeName("PIPE_FLITE");

    if (m_flagFlite3aaOTF == true) {
        m_pipes[INDEX(PIPE_3AA)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_3AA)]);
        m_pipes[INDEX(PIPE_3AA)]->setPipeId(PIPE_3AA);
        m_pipes[INDEX(PIPE_3AA)]->setPipeName("PIPE_3AA");

        m_pipes[INDEX(PIPE_ISP)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_ISP)]);
        m_pipes[INDEX(PIPE_ISP)]->setPipeId(PIPE_ISP);
        m_pipes[INDEX(PIPE_ISP)]->setPipeName("PIPE_ISP");
    } else {
        m_pipes[INDEX(PIPE_3AA)] = (ExynosCameraPipe*)new ExynosCameraPipe3AA(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_3AA)]);
        m_pipes[INDEX(PIPE_3AA)]->setPipeId(PIPE_3AA);
        m_pipes[INDEX(PIPE_3AA)]->setPipeName("PIPE_3AA");

        m_pipes[INDEX(PIPE_ISP)] = (ExynosCameraPipe*)new ExynosCameraPipeISP(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_ISP)]);
        m_pipes[INDEX(PIPE_ISP)]->setPipeId(PIPE_ISP);
        m_pipes[INDEX(PIPE_ISP)]->setPipeName("PIPE_ISP");
    }

    if (m_parameters->getHWVdisMode()) {
        m_pipes[INDEX(PIPE_DIS)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_DIS)]);
        m_pipes[INDEX(PIPE_DIS)]->setPipeId(PIPE_DIS);
        m_pipes[INDEX(PIPE_DIS)]->setPipeName("PIPE_DIS");
    }

    m_pipes[INDEX(PIPE_GSC)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_GSC)]);
    m_pipes[INDEX(PIPE_GSC)]->setPipeId(PIPE_GSC);
    m_pipes[INDEX(PIPE_GSC)]->setPipeName("PIPE_GSC");

    m_pipes[INDEX(PIPE_GSC_VIDEO)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_GSC_VIDEO)]);
    m_pipes[INDEX(PIPE_GSC_VIDEO)]->setPipeId(PIPE_GSC_VIDEO);
    m_pipes[INDEX(PIPE_GSC_VIDEO)]->setPipeName("PIPE_GSC_VIDEO");

    if (m_supportReprocessing == false) {
        m_pipes[INDEX(PIPE_GSC_PICTURE)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_GSC_PICTURE)]);
        m_pipes[INDEX(PIPE_GSC_PICTURE)]->setPipeId(PIPE_GSC_PICTURE);
        m_pipes[INDEX(PIPE_GSC_PICTURE)]->setPipeName("PIPE_GSC_PICTURE");

        m_pipes[INDEX(PIPE_JPEG)] = (ExynosCameraPipe*)new ExynosCameraPipeJpeg(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_JPEG)]);
        m_pipes[INDEX(PIPE_JPEG)]->setPipeId(PIPE_JPEG);
        m_pipes[INDEX(PIPE_JPEG)]->setPipeName("PIPE_JPEG");
    }

    /* flite pipe initialize */
    ret = m_pipes[INDEX(PIPE_FLITE)]->create(m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_FLITE));

    /* FAST AE init before ISP setInput */
    if ( active == true &&
        m_parameters->getUseFastenAeStable() == true &&
        m_parameters->getCameraId() == CAMERA_ID_BACK &&
        m_parameters->getDualMode() == false &&
        m_parameters->getRecordingHint() == false &&
        m_parameters->getIsFirstStartFlag() == true) {

        int sensorMarginW, sensorMarginH;
        int32_t sensorIds[MAX_NODE];
        for (int i = 0; i < MAX_NODE; i++)
            sensorIds[i] = -1;

        camera_pipe_info_t pipeInfo[MAX_NODE];
        camera_pipe_info_t nullPipeInfo;
        ExynosRect tempRect;
        int hwSensorW = 0, hwSensorH = 0;
        int bayerFormat = CAMERA_BAYER_FORMAT;

        m_parameters->getFastenAeStableSensorSize(&hwSensorW, &hwSensorH);

        CLOGI("INFO(%s[%d]): hwSensorSize(%dx%d)", __FUNCTION__, __LINE__, hwSensorW, hwSensorH);

        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;


        struct v4l2_streamparm streamParam;
        uint32_t frameRate = 0;

        frameRate  = FASTEN_AE_FPS;

        /* setParam for Frame rate : must after setInput on Flite */
        memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

        streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        streamParam.parm.capture.timeperframe.numerator   = 1;
        streamParam.parm.capture.timeperframe.denominator = frameRate;
        CLOGI("INFO(%s[%d]:set framerate (denominator=%d)", __FUNCTION__, __LINE__, frameRate);
        ret = setParam(&streamParam, PIPE_FLITE);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE setParam fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        /* FLITE pipe */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        pipeInfo[0].rectInfo = tempRect;
        pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
        pipeInfo[0].bufInfo.count = 10;
        /* per frame info */
        pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            /* packed bayer bytesPerPlane */
            pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 2;
        }
        else
#endif
        {
            /* packed bayer bytesPerPlane */
            pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 8 / 5;
        }
#endif

        ret = m_pipes[INDEX(PIPE_FLITE)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_FLITE)]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    }

    /* ISP pipe initialize */
    ret = m_pipes[INDEX(PIPE_ISP)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):ISP create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_ISP));

    /* 3AA pipe initialize */
    ret = m_pipes[INDEX(PIPE_3AA)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_3AA));

    /* DIS pipe initialize */
    if (m_parameters->getHWVdisMode()) {
        ret = m_pipes[INDEX(PIPE_DIS)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_DIS));
    }
    /* GSC_PREVIEW pipe initialize */
    ret = m_pipes[INDEX(PIPE_GSC)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):GSC create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC));

    ret = m_pipes[INDEX(PIPE_GSC_VIDEO)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_GSC_VIDEO create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC_VIDEO));

    if (m_supportReprocessing == false) {
        /* GSC_PICTURE pipe initialize */
        ret = m_pipes[INDEX(PIPE_GSC_PICTURE)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):GSC_PICTURE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC_PICTURE));

        /* JPEG pipe initialize */
        ret = m_pipes[INDEX(PIPE_JPEG)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):JPEG create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_JPEG));
    }

    /* EOS */
    ret = m_pipes[INDEX(PIPE_3AA)]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", __FUNCTION__, __LINE__, PIPE_3AA, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    m_setCreate(true);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::precreate(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_setupConfig();

    int ret = 0;
    int32_t nodeNums[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++)
        nodeNums[i] = -1;

    m_pipes[INDEX(PIPE_FLITE)] = (ExynosCameraPipe*)new ExynosCameraPipeFlite(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_FLITE)]);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeId(PIPE_FLITE);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeName("PIPE_FLITE");

    if (m_flagFlite3aaOTF == true) {
        m_pipes[INDEX(PIPE_3AA)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_3AA)]);
        m_pipes[INDEX(PIPE_3AA)]->setPipeId(PIPE_3AA);
        m_pipes[INDEX(PIPE_3AA)]->setPipeName("PIPE_3AA");

        m_pipes[INDEX(PIPE_ISP)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_ISP)]);
        m_pipes[INDEX(PIPE_ISP)]->setPipeId(PIPE_ISP);
        m_pipes[INDEX(PIPE_ISP)]->setPipeName("PIPE_ISP");
    } else {
        m_pipes[INDEX(PIPE_3AA)] = (ExynosCameraPipe*)new ExynosCameraPipe3AA(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_3AA)]);
        m_pipes[INDEX(PIPE_3AA)]->setPipeId(PIPE_3AA);
        m_pipes[INDEX(PIPE_3AA)]->setPipeName("PIPE_3AA");

        m_pipes[INDEX(PIPE_ISP)] = (ExynosCameraPipe*)new ExynosCameraPipeISP(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_ISP)]);
        m_pipes[INDEX(PIPE_ISP)]->setPipeId(PIPE_ISP);
        m_pipes[INDEX(PIPE_ISP)]->setPipeName("PIPE_ISP");
    }

    if (m_parameters->getHWVdisMode()) {
        m_pipes[INDEX(PIPE_DIS)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_DIS)]);
        m_pipes[INDEX(PIPE_DIS)]->setPipeId(PIPE_DIS);
        m_pipes[INDEX(PIPE_DIS)]->setPipeName("PIPE_DIS");
    }

    m_pipes[INDEX(PIPE_GSC)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_GSC)]);
    m_pipes[INDEX(PIPE_GSC)]->setPipeId(PIPE_GSC);
    m_pipes[INDEX(PIPE_GSC)]->setPipeName("PIPE_GSC");

    m_pipes[INDEX(PIPE_GSC_VIDEO)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_GSC_VIDEO)]);
    m_pipes[INDEX(PIPE_GSC_VIDEO)]->setPipeId(PIPE_GSC_VIDEO);
    m_pipes[INDEX(PIPE_GSC_VIDEO)]->setPipeName("PIPE_GSC_VIDEO");

    if (m_supportReprocessing == false) {
        m_pipes[INDEX(PIPE_GSC_PICTURE)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_GSC_PICTURE)]);
        m_pipes[INDEX(PIPE_GSC_PICTURE)]->setPipeId(PIPE_GSC_PICTURE);
        m_pipes[INDEX(PIPE_GSC_PICTURE)]->setPipeName("PIPE_GSC_PICTURE");

        m_pipes[INDEX(PIPE_JPEG)] = (ExynosCameraPipe*)new ExynosCameraPipeJpeg(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_JPEG)]);
        m_pipes[INDEX(PIPE_JPEG)]->setPipeId(PIPE_JPEG);
        m_pipes[INDEX(PIPE_JPEG)]->setPipeName("PIPE_JPEG");
    }

    /* flite pipe initialize */
    ret = m_pipes[INDEX(PIPE_FLITE)]->create(m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_FLITE));

    /* FAST AE init before ISP setInput */
    if (m_parameters->getUseFastenAeStable() == true &&
        m_parameters->getCameraId() == CAMERA_ID_BACK &&
        m_parameters->getDualMode() == false &&
        m_parameters->getRecordingHint() == false &&
        m_parameters->getIsFirstStartFlag() == true) {

        int sensorMarginW, sensorMarginH;
        int32_t sensorIds[MAX_NODE];
        for (int i = 0; i < MAX_NODE; i++)
            sensorIds[i] = -1;

        camera_pipe_info_t pipeInfo[MAX_NODE];
        camera_pipe_info_t nullPipeInfo;
        ExynosRect tempRect;
        int hwSensorW = 0, hwSensorH = 0;
        int bayerFormat = CAMERA_BAYER_FORMAT;

        m_parameters->getFastenAeStableSensorSize(&hwSensorW, &hwSensorH);

        CLOGI("INFO(%s[%d]): hwSensorSize(%dx%d)", __FUNCTION__, __LINE__, hwSensorW, hwSensorH);

        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;


        /* setParam for Frame rate : must after setInput on Flite */
        struct v4l2_streamparm streamParam;
        uint32_t frameRate = 0;

        frameRate  = FASTEN_AE_FPS;

        memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

        streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        streamParam.parm.capture.timeperframe.numerator   = 1;
        streamParam.parm.capture.timeperframe.denominator = frameRate;
        CLOGI("INFO(%s[%d]:set framerate (denominator=%d)", __FUNCTION__, __LINE__, frameRate);
        ret = setParam(&streamParam, PIPE_FLITE);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE setParam fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        /* FLITE pipe */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        pipeInfo[0].rectInfo = tempRect;
        pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
        pipeInfo[0].bufInfo.count = 10;
        /* per frame info */
        pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            /* packed bayer bytesPerPlane */
            pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 2;
        }
        else
#endif
        {
            /* packed bayer bytesPerPlane */
            pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 8 / 5;
        }
#endif

        ret = m_pipes[INDEX(PIPE_FLITE)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_FLITE)]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    }

    /* ISP pipe initialize */
    ret = m_pipes[INDEX(PIPE_ISP)]->precreate();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):ISP create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_ISP));

    /* 3AA pipe initialize */
    ret = m_pipes[INDEX(PIPE_3AA)]->precreate();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_3AA));

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::postcreate(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    if (m_flagFlite3aaOTF == true) {
        /* 3AA_ISP pipe initialize */
        ret = m_pipes[INDEX(PIPE_3AA)]->postcreate();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):3AA_ISP postcreate fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) postcreated", __FUNCTION__, INDEX(PIPE_3AA_ISP));
    }

    /* ISP pipe initialize */
    ret = m_pipes[INDEX(PIPE_ISP)]->postcreate();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):ISP create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_ISP));

    if (m_parameters->getHWVdisMode()) {
        /* DIS pipe initialize */
        ret = m_pipes[INDEX(PIPE_DIS)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_DIS));
    }
    /* GSC pipe initialize */
    ret = m_pipes[INDEX(PIPE_GSC)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):GSC create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC));

    ret = m_pipes[INDEX(PIPE_GSC_VIDEO)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):GSC create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC_VIDEO));

    if (m_supportReprocessing == false) {
        /* GSC_PICTURE pipe initialize */
        ret = m_pipes[INDEX(PIPE_GSC_PICTURE)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):GSC_PICTURE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC_PICTURE));

        /* JPEG pipe initialize */
        ret = m_pipes[INDEX(PIPE_JPEG)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):JPEG create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_JPEG));
    }

    /* EOS */
    ret = m_pipes[INDEX(PIPE_3AA)]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", __FUNCTION__, __LINE__, PIPE_3AA, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    m_setCreate(true);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::fastenAeStable(int32_t numFrames, ExynosCameraBuffer *buffers)
{
    CLOGI("INFO(%s[%d]): Start", __FUNCTION__, __LINE__);

    int ret = 0;
    status_t totalRet = NO_ERROR;

    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrameEntity *newEntity = NULL;
    ExynosCameraList<ExynosCameraFrame *> instantQ;

    /* TODO 1. setup pipes for 120FPS */
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int32_t nodeNums[MAX_NODE];
    int32_t sensorIds[MAX_NODE];
    int32_t secondarySensorIds[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++) {
        nodeNums[i] = -1;
        sensorIds[i] = -1;
        secondarySensorIds[i] = -1;
    }

    ExynosRect tempRect;
    int hwSensorW = 0, hwSensorH = 0;
    int hwPreviewW = 0, hwPreviewH = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    int previewFormat = m_parameters->getHwPreviewFormat();
    int hwVdisformat = m_parameters->getHWVdisFormat();
    struct ExynosConfigInfo *config = m_parameters->getConfig();
    ExynosRect bdsSize;
    uint32_t frameRate = 0;
    struct v4l2_streamparm streamParam;
    int perFramePos = 0;

#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    if (numFrames < 1) {
        CLOGW("WRN(%s[%d]): numFrames is %d, we skip fastenAeStable", __FUNCTION__, __LINE__, numFrames);
        return NO_ERROR;
    }

#if 0
    frameRate = 30;
    m_parameters->getMaxSensorSize(&maxSensorW, &maxSensorH);
    m_parameters->getMaxPreviewSize(&maxPreviewW, &maxPreviewH);
    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);
#else
    frameRate  = FASTEN_AE_FPS;
    m_parameters->getFastenAeStableSensorSize(&hwSensorW, &hwSensorH);
    m_parameters->getFastenAeStableBdsSize(&hwPreviewW, &hwPreviewH);
#endif

    m_parameters->getPreviewBdsSize(&bdsSize);

    CLOGI("INFO(%s[%d]): hwSensorSize(%dx%d)", __FUNCTION__, __LINE__, hwSensorW, hwSensorH);

    /* FLITE pipe */
    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;

    /* setParam for Frame rate : must after setInput on Flite */
    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = frameRate;
    CLOGI("INFO(%s[%d]:set framerate (denominator=%d)", __FUNCTION__, __LINE__, frameRate);
    ret = setParam(&streamParam, PIPE_FLITE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE setParam fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return INVALID_OPERATION;
    }

    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[0].rectInfo = tempRect;
    pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[0].bufInfo.count = numFrames;
    /* per frame info */
    pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        /* packed bayer bytesPerPlane */
        pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 2;
    }
    else
#endif
    {
        /* packed bayer bytesPerPlane */
        pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 8 / 5;
    }
#endif

    ret = m_pipes[INDEX(PIPE_FLITE)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* setParam for Frame rate : must after setInput on Flite */
    int bnsScaleRatio = 1000;
    ret = m_pipes[INDEX(PIPE_FLITE)]->setControl(V4L2_CID_IS_S_BNS, bnsScaleRatio);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): set BNS(%d) fail, ret(%d)", __FUNCTION__, __LINE__, bnsScaleRatio, ret);
    }

    ret = m_setDeviceInfo();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setDeviceInfo() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = m_initPipesFastenAeStable(numFrames,
                                       hwSensorW, hwSensorW,
                                       hwPreviewW, hwPreviewH);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_initPipesFastenAeStable(%d) fail",
            __FUNCTION__, __LINE__);
        return ret;
    }

    for (int i = 0; i < numFrames; i++) {
        /* 2. generate instant frames */
        newFrame = m_frameMgr->createFrame(m_parameters, i);

        ret = m_initFrameMetadata(newFrame);
        if (ret < 0)
            CLOGE("(%s[%d]): frame(%d) metadata initialize fail", __FUNCTION__, __LINE__, i);

        newEntity = new ExynosCameraFrameEntity(PIPE_3AA, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        newFrame->addSiblingEntity(NULL, newEntity);
        newFrame->setNumRequestPipe(1);

        newEntity->setSrcBuf(buffers[i]);

        /* set metadata for instant on */
        camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buffers[i].addr[1]);

        if (shot_ext != NULL) {
            int aeRegionX = (hwSensorW) / 2;
            int aeRegionY = (hwSensorH) / 2;

            newFrame->getMetaData(shot_ext);
            m_parameters->duplicateCtrlMetadata((void *)shot_ext);
            m_activityControl->activityBeforeExecFunc(newEntity->getPipeId(), (void *)&buffers[i]);

#ifdef SR_CAPTURE
            /* setfile setting */
            int setfile = 0;
            int yuvRange = 0;
            m_parameters->getSetfileYuvRange(0, &setfile, &yuvRange);
            ALOGV("INFO(%s[%d]):setfile(%d)", __FUNCTION__, __LINE__, setfile);
            setMetaSetfile(shot_ext, setfile);
#endif

            /* set metadata for instant on */
            shot_ext->shot.ctl.scaler.cropRegion[0] = 0;
            shot_ext->shot.ctl.scaler.cropRegion[1] = 0;
            shot_ext->shot.ctl.scaler.cropRegion[2] = hwPreviewW;
            shot_ext->shot.ctl.scaler.cropRegion[3] = hwPreviewH;

            setMetaCtlAeTargetFpsRange(shot_ext, FASTEN_AE_FPS, FASTEN_AE_FPS);
            setMetaCtlSensorFrameDuration(shot_ext, (uint64_t)((1000 * 1000 * 1000) / (uint64_t)FASTEN_AE_FPS));

            /* set afMode into INFINITY */
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_CANCEL;
            shot_ext->shot.ctl.aa.vendor_afmode_option &= (0 << AA_AFMODE_OPTION_BIT_MACRO);

            setMetaCtlAeRegion(shot_ext, aeRegionX, aeRegionY, aeRegionX, aeRegionY, 0);

            enum NODE_TYPE t3apNodeType = getNodeType(PIPE_3AP);
            int t3apNodeNum = m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[t3apNodeType];

            if (m_flag3aaIspOTF == true)
                t3apNodeNum = m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[t3apNodeType];

            if (t3apNodeNum <= 0) {
                CLOGE("ERR(%s[%d]): invalid t3apNodeNum(%d). so fail", __FUNCTION__, __LINE__, t3apNodeNum);
                ret = INVALID_OPERATION;
                goto cleanup;
            }

            setMetaNodeCaptureVideoID(shot_ext, t3apNodeType, t3apNodeNum - FIMC_IS_VIDEO_BAS_NUM);
            setMetaNodeCaptureRequest(shot_ext, t3apNodeType, 1);
            setMetaNodeCaptureOutputSize(shot_ext, t3apNodeType, 0, 0, hwPreviewW, hwPreviewH);
        }

        /* 3. push instance frames to pipe */
        ret = pushFrameToPipe(&newFrame, newEntity->getPipeId());
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): pushFrameToPipeFail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto cleanup;
        }
        CLOGD("DEBUG(%s[%d]): Instant shot - FD(%d, %d)", __FUNCTION__, __LINE__, buffers[i].fd[0], buffers[i].fd[1]);

        instantQ.pushProcessQ(&newFrame);
    }

    /* 4. pipe instant on */
    ret = m_pipes[INDEX(PIPE_FLITE)]->instantOn(0);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): FLITE On fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto cleanup;
    }

    if (newEntity == NULL)
        goto cleanup;

    if (m_parameters->getTpuEnabledMode() == true) {
        ret = m_pipes[INDEX(PIPE_DIS)]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto cleanup;
        }
    }

    ret = m_pipes[INDEX(newEntity->getPipeId())]->instantOn(numFrames);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): 3AA On fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto cleanup;
    }

    /* 5. setControl to sensor instant on */
    ret = m_pipes[INDEX(PIPE_FLITE)]->setControl(V4L2_CID_IS_S_STREAM, (1 | numFrames << SENSOR_INSTANT_SHIFT));
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): instantOn fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto cleanup;
    }

cleanup:
    totalRet |= ret;

    /* 6. pipe instant off */
    ret = m_pipes[INDEX(PIPE_FLITE)]->instantOff();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): FLITE Off fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    /* 3AA force done */
    ret = m_pipes[newEntity->getPipeId()]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    ret = m_pipes[INDEX(newEntity->getPipeId())]->instantOff();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): 3AA Off fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        if (m_pipes[INDEX(PIPE_DIS)]->flagStart() == true) {
            ret = m_pipes[INDEX(PIPE_DIS)]->stop();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):PIPE_DIS Off fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            }
        }
    }

    /* setParam for Frame rate : must after setInput on Flite */
    /* rollback framerate after fastenfeenable done */
    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = 30;
    CLOGI("INFO(%s[%d]:set framerate (denominator=%d)", __FUNCTION__, __LINE__, frameRate);
    ret = setParam(&streamParam, PIPE_FLITE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE setParam fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return INVALID_OPERATION;
    }

    newFrame = NULL;

    /* clean up all frames */
    for (int i = 0; i < numFrames; i++) {
        if (instantQ.getSizeOfProcessQ() == 0)
            break;

        ret = instantQ.popProcessQ(&newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): pop instantQ fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            continue;
        }
        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]): newFrame is NULL,", __FUNCTION__, __LINE__);
            continue;
        }
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

    CLOGI("INFO(%s[%d]): Done", __FUNCTION__, __LINE__);

    ret |= totalRet;
    return ret;
}

status_t ExynosCameraFrameFactoryPreview::m_fillNodeGroupInfo(ExynosCameraFrame *frame)
{
    camera2_node_group node_group_info_3aa, node_group_info_isp, node_group_info_dis;
    int zoom = m_parameters->getZoomLevel();
    int previewW = 0, previewH = 0;
    int pictureW = 0, pictureH = 0;
    ExynosRect bnsSize;       /* == bayerCropInputSize */
    ExynosRect bayerCropSize;
    ExynosRect bdsSize;
    int perFramePos = 0;
    bool tpu = false;
    bool dis = false;

    m_parameters->getHwPreviewSize(&previewW, &previewH);
    m_parameters->getPictureSize(&pictureW, &pictureH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
    m_parameters->getPreviewBdsSize(&bdsSize);
    tpu = m_parameters->getTpuEnabledMode();
    dis = m_parameters->getHWVdisMode();

    memset(&node_group_info_3aa, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_isp, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_dis, 0x0, sizeof(camera2_node_group));

    /* should add this request value in FrameFactory */
    /* 3AA */
    node_group_info_3aa.leader.request = 1;

    /* 3AC */
    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AC_POS : PERFRAME_FRONT_3AC_POS;
    node_group_info_3aa.capture[perFramePos].request = m_request3AC;

    /* 3AP */
    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
    node_group_info_3aa.capture[perFramePos].request = m_request3AP;

    /* should add this request value in FrameFactory */
    /* ISP */
    node_group_info_isp.leader.request = 1;

    /* SCC */
    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCC_POS : PERFRAME_FRONT_SCC_POS;

    if (m_supportSCC == true)
        node_group_info_isp.capture[perFramePos].request = m_requestSCC;
    else
        node_group_info_isp.capture[perFramePos].request = m_requestISPC;

    /* DIS */
    memcpy(&node_group_info_dis, &node_group_info_isp, sizeof (camera2_node_group));

    if (tpu == true) {
        /* ISPP */
        if (m_requestISPP == true) {
            perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPP_POS : PERFRAME_FRONT_ISPP_POS;
            node_group_info_isp.capture[perFramePos].request = m_requestISPP;
        }

        /* SCP */
        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;
        node_group_info_dis.capture[perFramePos].request = m_requestSCP;
    } else {
        /* SCP */
        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;

        if (m_flag3aaIspOTF == true)
            node_group_info_3aa.capture[perFramePos].request = m_requestSCP;
        else
            node_group_info_isp.capture[perFramePos].request = m_requestSCP;
    }

    ExynosCameraNodeGroup3AA::updateNodeGroupInfo(
        m_cameraId,
        &node_group_info_3aa,
        bayerCropSize,
        bdsSize,
        previewW, previewH,
        pictureW, pictureH);

    ExynosCameraNodeGroupISP::updateNodeGroupInfo(
        m_cameraId,
        &node_group_info_isp,
        bayerCropSize,
        bdsSize,
        previewW, previewH,
        pictureW, pictureH,
        dis);

    ExynosCameraNodeGroupDIS::updateNodeGroupInfo(
        m_cameraId,
        &node_group_info_dis,
        bayerCropSize,
        bdsSize,
        previewW, previewH,
        pictureW, pictureH,
        dis);

    frame->storeNodeGroupInfo(&node_group_info_3aa, PERFRAME_INFO_3AA, zoom);
    frame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP, zoom);
    frame->storeNodeGroupInfo(&node_group_info_dis, PERFRAME_INFO_DIS, zoom);

    return NO_ERROR;
}

ExynosCameraFrame *ExynosCameraFrameFactoryPreview::createNewFrame(void)
{
    int ret = 0;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    ExynosCameraFrame *frame = m_frameMgr->createFrame(m_parameters, m_frameCount, FRAME_TYPE_PREVIEW);

    int requestEntityCount = 0;

    ret = m_initFrameMetadata(frame);
    if (ret < 0)
        CLOGE("(%s[%d]): frame(%d) metadata initialize fail", __FUNCTION__, __LINE__, m_frameCount);

    if (m_flagFlite3aaOTF == true) {
        if (m_requestFLITE) {
            /* set flite pipe to linkageList */
            newEntity[INDEX(PIPE_FLITE)] = new ExynosCameraFrameEntity(PIPE_FLITE, ENTITY_TYPE_OUTPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_FLITE)]);
            requestEntityCount++;
        }

        /* set 3AA_ISP pipe to linkageList */
        newEntity[INDEX(PIPE_3AA)] = new ExynosCameraFrameEntity(PIPE_3AA, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_3AA)]);
        requestEntityCount++;

        if (m_requestDIS == true) {
            if (m_flag3aaIspOTF == true) {
                /* set DIS pipe to linkageList */
                newEntity[INDEX(PIPE_DIS)] = new ExynosCameraFrameEntity(PIPE_DIS, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
                frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_DIS)], INDEX(PIPE_ISPP));
                requestEntityCount++;
            } else {
                /* set ISP pipe to linkageList */
                newEntity[INDEX(PIPE_ISP)] = new ExynosCameraFrameEntity(PIPE_ISP, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
                frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_ISP)], INDEX(PIPE_3AP));
                requestEntityCount++;

                /* set DIS pipe to linkageList */
                newEntity[INDEX(PIPE_DIS)] = new ExynosCameraFrameEntity(PIPE_DIS, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
                frame->addChildEntity(newEntity[INDEX(PIPE_ISP)], newEntity[INDEX(PIPE_DIS)], INDEX(PIPE_ISPP));
                requestEntityCount++;
            }
        } else {
            if (m_flag3aaIspOTF == true) {
                /* skip ISP pipe to linkageList */
            } else {
                /* set ISP pipe to linkageList */
                newEntity[INDEX(PIPE_ISP)] = new ExynosCameraFrameEntity(PIPE_ISP, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
                frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_ISP)], INDEX(PIPE_3AP));
                requestEntityCount++;
            }
        }
    } else {
        /* set flite pipe to linkageList */
        newEntity[INDEX(PIPE_FLITE)] = new ExynosCameraFrameEntity(PIPE_FLITE, ENTITY_TYPE_OUTPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_FLITE)]);

        /* set 3AA pipe to linkageList */
        newEntity[INDEX(PIPE_3AA)] = new ExynosCameraFrameEntity(PIPE_3AA, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addChildEntity(newEntity[INDEX(PIPE_FLITE)], newEntity[INDEX(PIPE_3AA)]);

        /* set ISP pipe to linkageList */
        newEntity[INDEX(PIPE_ISP)] = new ExynosCameraFrameEntity(PIPE_ISP, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_ISP)]);

        /* set DIS pipe to linkageList */
        if (m_requestDIS == true) {
            newEntity[INDEX(PIPE_DIS)] = new ExynosCameraFrameEntity(PIPE_DIS, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
            frame->addChildEntity(newEntity[INDEX(PIPE_ISP)], newEntity[INDEX(PIPE_DIS)]);
        }

        /* flite, 3aa, isp, dis as one. */
        requestEntityCount++;
    }

    if (m_supportReprocessing == false) {
        /* set GSC-Picture pipe to linkageList */
        newEntity[INDEX(PIPE_GSC_PICTURE)] = new ExynosCameraFrameEntity(PIPE_GSC_PICTURE, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_PICTURE)]);
    }

    /* set GSC pipe to linkageList */
    newEntity[INDEX(PIPE_GSC)] = new ExynosCameraFrameEntity(PIPE_GSC, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC)]);

    newEntity[INDEX(PIPE_GSC_VIDEO)] = new ExynosCameraFrameEntity(PIPE_GSC_VIDEO, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_VIDEO)]);

    if (m_supportReprocessing == false) {
        /* set JPEG pipe to linkageList */
        newEntity[INDEX(PIPE_JPEG)] = new ExynosCameraFrameEntity(PIPE_JPEG, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_JPEG)]);
    }

    ret = m_initPipelines(frame);
    if (ret < 0) {
        CLOGE("ERR(%s):m_initPipelines fail, ret(%d)", __FUNCTION__, ret);
    }

    /* TODO: make it dynamic */
    frame->setNumRequestPipe(requestEntityCount);

    m_fillNodeGroupInfo(frame);

    m_frameCount++;

    return frame;
}

status_t ExynosCameraFrameFactoryPreview::initPipes(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;

    ret = m_initFlitePipe();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_initFlitePipe() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = m_setDeviceInfo();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setDeviceInfo(%d) fail", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = m_initPipes();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_initPipes() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    m_frameCount = 0;

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::preparePipes(void)
{
    int ret = 0;

    /* NOTE: Prepare for 3AA is moved after ISP stream on */

    if (m_requestFLITE) {
        ret = m_pipes[INDEX(PIPE_FLITE)]->prepare();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE prepare fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::startPipes(void)
{
    int ret = 0;

    if (m_parameters->getTpuEnabledMode() == true) {
        ret = m_pipes[INDEX(PIPE_DIS)]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_pipes[INDEX(PIPE_ISP)]->start();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):ISP start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_3AA)]->start();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->start();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    if (m_flagFlite3aaOTF == true) {
        /* Here is doing 3AA prepare(qbuf) */
        ret = m_pipes[INDEX(PIPE_3AA)]->prepare();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):3AA prepare fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->sensorStream(true);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE sensorStream on fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    CLOGI("INFO(%s[%d]):Starting Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::startInitialThreads(void)
{
    int ret = 0;

    CLOGI("INFO(%s[%d]):start pre-ordered initial pipe thread", __FUNCTION__, __LINE__);

    if (m_requestFLITE) {
        ret = startThread(PIPE_FLITE);
        if (ret < 0)
            return ret;
    }

    ret = startThread(PIPE_3AA);
    if (ret < 0)
        return ret;

    if (m_parameters->is3aaIspOtf() == false) {
        ret = startThread(PIPE_ISP);
        if (ret < 0)
            return ret;
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        ret = startThread(PIPE_DIS);
        if (ret < 0)
            return ret;
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::setStopFlag(void)
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;

    ret = m_pipes[INDEX(PIPE_FLITE)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_3AA)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_3AA)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_ISP)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_ISP)]->setStopFlag();

    if (m_parameters->getHWVdisMode()) {
        if (m_pipes[INDEX(PIPE_DIS)]->flagStart() == true)
            ret = m_pipes[INDEX(PIPE_DIS)]->setStopFlag();
    }
    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::stopPipes(void)
{
    int ret = 0;

    if (m_pipes[INDEX(PIPE_DIS)] != NULL &&
        m_pipes[INDEX(PIPE_DIS)]->flagStartThread() == true) {
        ret = m_pipes[INDEX(PIPE_DIS)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_pipes[INDEX(PIPE_3AA)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_3AA)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):3AA stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    /* stream off for ISP */
    if (m_pipes[INDEX(PIPE_ISP)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_ISP)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):ISP stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_requestFLITE) {
        ret = m_pipes[INDEX(PIPE_FLITE)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->sensorStream(false);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE sensorStream off fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* 3AA force done */
    ret = m_pipes[INDEX(PIPE_3AA)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_3AA force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        /* return INVALID_OPERATION; */
    }

    /* stream off for 3AA */
    ret = m_pipes[INDEX(PIPE_3AA)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* ISP force done */
    ret = m_pipes[INDEX(PIPE_ISP)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_ISP force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        /* return INVALID_OPERATION; */
    }

    /* stream off for ISP */
    ret = m_pipes[INDEX(PIPE_ISP)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):ISP stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    if (m_parameters->getHWVdisMode()) {
        if (m_pipes[INDEX(PIPE_DIS)]->flagStart() == true) {
            /* DIS force done */
            ret = m_pipes[INDEX(PIPE_DIS)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):PIPE_DIS force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                /* return INVALID_OPERATION; */
            }

            ret = m_pipes[INDEX(PIPE_DIS)]->stop();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):DIS stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }

    CLOGI("INFO(%s[%d]):Stopping  Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void ExynosCameraFrameFactoryPreview::m_init(void)
{
    m_supportReprocessing = false;
    m_flagFlite3aaOTF = false;
    m_supportSCC = false;
    m_supportPureBayerReprocessing = false;
    m_flagReprocessing = false;
}

status_t ExynosCameraFrameFactoryPreview::m_setupConfig()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    int32_t *nodeNums = NULL;
    int32_t *controlId = NULL;
    int32_t *secondaryControlId = NULL;
    int32_t *prevNode = NULL;

    m_flagFlite3aaOTF = m_parameters->isFlite3aaOtf();
    m_flag3aaIspOTF = m_parameters->is3aaIspOtf();
    m_supportReprocessing = m_parameters->isReprocessing();
    m_supportSCC = isOwnScc(m_cameraId);

    if (m_parameters->getRecordingHint() == true) {
        m_supportPureBayerReprocessing = (m_cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_RECORDING : USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING;
    } else {
        m_supportPureBayerReprocessing = (m_cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING : USE_PURE_BAYER_REPROCESSING_FRONT;
    }

    m_flagReprocessing = false;

    if (m_supportReprocessing == false) {
        if (m_supportSCC == true)
            m_requestSCC = 1;
        else
            m_requestISPC = 1;
    }

    if (m_flag3aaIspOTF == true) {
        m_request3AP = 0;
        m_requestISP = 0;
    } else {
        m_request3AP = 1;
        m_requestISP = 1;
    }

    nodeNums = m_nodeNums[INDEX(PIPE_FLITE)];
    nodeNums[OUTPUT_NODE] = -1;
    nodeNums[CAPTURE_NODE_1] = m_getFliteNodenum();
    nodeNums[CAPTURE_NODE_2] = -1;
    controlId = m_sensorIds[INDEX(PIPE_FLITE)];
    controlId[CAPTURE_NODE_1] = m_getSensorId(nodeNums[CAPTURE_NODE_1], m_flagReprocessing);

    ret = m_setDeviceInfo();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setDeviceInfo(%d, %d) fail", __FUNCTION__, __LINE__);
        return ret;
    }

    nodeNums = m_nodeNums[INDEX(PIPE_GSC)];
    nodeNums[OUTPUT_NODE] = PREVIEW_GSC_NODE_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    nodeNums = m_nodeNums[INDEX(PIPE_GSC_VIDEO)];
    nodeNums[OUTPUT_NODE] = VIDEO_GSC_NODE_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    nodeNums = m_nodeNums[INDEX(PIPE_GSC_PICTURE)];
    nodeNums[OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    nodeNums = m_nodeNums[INDEX(PIPE_JPEG)];
    nodeNums[OUTPUT_NODE] = -1;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    return NO_ERROR;
}

}; /* namespace android */

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

#ifndef EXYNOS_CAMERA_PARAMETERS_H
#define EXYNOS_CAMERA_PARAMETERS_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>

#include <videodev2.h>
#include <videodev2_exynos_media.h>
#include <videodev2_exynos_camera.h>

#include "ExynosCameraSensorInfo.h"
#include "ExynosCameraCounter.h"
#include "ExynosCameraConfig.h"
#include "fimc-is-metadata.h"
#include "ExynosRect.h"
#include "exynos_format.h"
#include "ExynosExif.h"
#include "ExynosCameraUtils.h"
#include "ExynosCameraUtilsModule.h"
#include "ExynosCameraActivityControl.h"
#include "ExynosCameraAutoTimer.h"

#define STATE_REG_NEED_LLS              (1<<18)
#define STATE_REG_ZOOM_INDOOR           (1<<16)
#define STATE_REG_ZOOM_OUTDOOR          (1<<14)
#define STATE_REG_ZOOM                  (1<<12)
#define STATE_REG_RTHDR                 (1<<10)
#define STATE_REG_RECORDINGHINT         (1<<8)
#define STATE_REG_DUAL_RECORDINGHINT    (1<<6)
#define STATE_REG_UHD_RECORDING         (1<<4)
#define STATE_REG_DUAL_MODE             (1<<2)
#define STATE_REG_FLAG_REPROCESSING     (1)


#define STATE_STILL_PREVIEW                     (0)

#define STATE_STILL_CAPTURE                     (STATE_REG_FLAG_REPROCESSING)
#define STATE_STILL_CAPTURE_ZOOM                (STATE_REG_FLAG_REPROCESSING|STATE_REG_ZOOM)
#define STATE_STILL_CAPTURE_ZOOM_OUTDOOR        (STATE_REG_FLAG_REPROCESSING|STATE_REG_ZOOM_OUTDOOR)
#define STATE_STILL_CAPTURE_ZOOM_INDOOR         (STATE_REG_FLAG_REPROCESSING|STATE_REG_ZOOM_INDOOR)
#define STATE_VIDEO_CAPTURE                     (STATE_REG_FLAG_REPROCESSING|STATE_REG_RECORDINGHINT)
#define STATE_STILL_CAPTURE_LLS                 (STATE_REG_FLAG_REPROCESSING|STATE_REG_NEED_LLS)

#define STATE_VIDEO                             (STATE_REG_RECORDINGHINT)

#define STATE_DUAL_VIDEO                        (STATE_REG_DUAL_RECORDINGHINT|STATE_REG_DUAL_MODE)
#define STATE_DUAL_VIDEO_CAPTURE                (STATE_REG_DUAL_RECORDINGHINT|STATE_REG_DUAL_MODE|STATE_REG_FLAG_REPROCESSING)
#define STATE_DUAL_STILL_PREVIEW                (STATE_REG_DUAL_MODE)
#define STATE_DUAL_STILL_CAPTURE                (STATE_REG_DUAL_MODE|STATE_REG_FLAG_REPROCESSING)

#define STATE_UHD_PREVIEW                       (STATE_REG_UHD_RECORDING)
#define STATE_UHD_VIDEO                         (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT)

#define STATE_UHD_PREVIEW_CAPTURE               (STATE_REG_UHD_RECORDING|STATE_REG_FLAG_REPROCESSING)
#define STATE_UHD_VIDEO_CAPTURE                 (STATE_REG_UHD_RECORDING|STATE_REG_RECORDINGHINT|STATE_REG_FLAG_REPROCESSING)

#define SIZE_RATIO(w, h)         ((w) * 10 / (h))

namespace android {

namespace CONFIG_MODE {
	enum MODE {
		NORMAL        = 0x00,
        HIGHSPEED_60,
		HIGHSPEED_120,
		MAX
    };
};

struct CONFIG_PIPE {
	uint32_t prepare[MAX_PIPE_NUM_REPROCESSING];
};

struct CONFIG_BUFFER {
    uint32_t num_bayer_buffers;
    uint32_t init_bayer_buffers;
    uint32_t num_3aa_buffers;
    uint32_t num_hwdis_buffers;
    uint32_t num_preview_buffers;
    uint32_t num_preview_cb_buffers;
    uint32_t num_picture_buffers;
    uint32_t num_reprocessing_buffers;
    uint32_t num_recording_buffers;
    uint32_t num_fastaestable_buffer;
    uint32_t reprocessing_bayer_hold_count;
    uint32_t front_num_bayer_buffers;
    uint32_t front_num_picture_buffers;
    uint32_t preview_buffer_margin;
};

struct CONFIG_BUFFER_PIPE {
    struct CONFIG_PIPE pipeInfo;
    struct CONFIG_BUFFER bufInfo;
};

struct ExynosConfigInfo {
    struct CONFIG_BUFFER_PIPE *current;
    struct CONFIG_BUFFER_PIPE info[CONFIG_MODE::MAX];
	uint32_t mode;
};

class ExynosCameraParameters {
public:
    /* Constructor */
    ExynosCameraParameters(int cameraId);

    /* Destructor */
    virtual ~ExynosCameraParameters();

    /* Create the instance */
    bool            create(int cameraId);
    /* Destroy the instance */
    bool            destroy(void);
    /* Check if the instance was created */
    bool            flagCreate(void);

    void            setDefaultCameraInfo(void);
    void            setDefaultParameter(void);

public:
    status_t        checkVisionMode(const CameraParameters& params);
    status_t        checkRecordingHint(const CameraParameters& params);
    status_t        checkDualMode(const CameraParameters& params);
    status_t        checkDualRecordingHint(const CameraParameters& params);
    status_t        checkEffectHint(const CameraParameters& params);
    /*
     * Check preview frame rate
     * both previewFpsRange(new API) and previewFrameRate(Old API)
     */
    status_t        checkPreviewFps(const CameraParameters& params);
    status_t        checkPreviewFpsRange(const CameraParameters& params);
    status_t        checkPreviewFrameRate(const CameraParameters& params);
    status_t        checkVideoSize(const CameraParameters& params);
    status_t        checkFastFpsMode(const CameraParameters& params);
    status_t        checkVideoStabilization(const CameraParameters& params);
    status_t        checkSWVdisMode(const CameraParameters& params);
    status_t        checkHWVdisMode(const CameraParameters& params);
    status_t        checkPreviewSize(const CameraParameters& params);
    status_t        checkPreviewFormat(const CameraParameters& params);
    status_t        checkPictureSize(const CameraParameters& params);
    status_t        checkPictureFormat(const CameraParameters& params);
    status_t        checkJpegQuality(const CameraParameters& params);
    status_t        checkThumbnailSize(const CameraParameters& params);
    status_t        checkThumbnailQuality(const CameraParameters& params);
    status_t        check3dnrMode(const CameraParameters& params);
    status_t        checkDrcMode(const CameraParameters& params);
    status_t        checkOdcMode(const CameraParameters& params);
    status_t        checkZoomLevel(const CameraParameters& params);
    status_t        checkRotation(const CameraParameters& params);
    status_t        checkAutoExposureLock(const CameraParameters& params);
    status_t        checkExposureCompensation(const CameraParameters& params);
    status_t        checkMeteringAreas(const CameraParameters& params);
    status_t        checkMeteringMode(const CameraParameters& params);
    status_t        checkAntibanding(const CameraParameters& params);
    status_t        checkSceneMode(const CameraParameters& params);
    status_t        checkFocusMode(const CameraParameters& params);
    status_t        checkFlashMode(const CameraParameters& params);
    status_t        checkWhiteBalanceMode(const CameraParameters& params);
    status_t        checkAutoWhiteBalanceLock(const CameraParameters& params);
    status_t        checkWbLevel(const CameraParameters& params);
    void            m_setWbLevel(int32_t value);
    int32_t         getWbLevel(void);
    status_t        checkFocusAreas(const CameraParameters& params);
    status_t        checkColorEffectMode(const CameraParameters& params);
    status_t        checkGpsAltitude(const CameraParameters& params);
    status_t        checkGpsLatitude(const CameraParameters& params);
    status_t        checkGpsLongitude(const CameraParameters& params);
    status_t        checkGpsProcessingMethod(const CameraParameters& params);
    status_t        checkGpsTimeStamp(const CameraParameters& params);
    status_t        checkCityId(const CameraParameters& params);
    status_t        checkWeatherId(const CameraParameters& params);
    status_t        checkBrightness(const CameraParameters& params);
    status_t        checkSaturation(const CameraParameters& params);
    status_t        checkSharpness(const CameraParameters& params);
    status_t        checkHue(const CameraParameters& params);
    status_t        checkIso(const CameraParameters& params);
    status_t        checkContrast(const CameraParameters& params);
    status_t        checkHdrMode(const CameraParameters& params);
    status_t        checkWdrMode(const CameraParameters& params);
    status_t        checkShotMode(const CameraParameters& params);
    status_t        checkAntiShake(const CameraParameters& params);
    status_t        checkVtMode(const CameraParameters& params);
    status_t        checkGamma(const CameraParameters& params);
    status_t        checkSlowAe(const CameraParameters& params);
    status_t        checkScalableSensorMode(const CameraParameters& params);
    status_t        checkImageUniqueId(const CameraParameters& params);
    status_t        checkSeriesShotMode(const CameraParameters& params);
#ifdef BURST_CAPTURE
    status_t        checkSeriesShotFilePath(const CameraParameters& params);
#endif

#ifdef SAMSUNG_DOF
    status_t	    checkMoveLens(const CameraParameters& params);
#endif

    status_t        calcPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcHighResolutionPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcRecordingGSCRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPictureRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPictureRect(int originW, int originH, ExynosRect *srcRect, ExynosRect *dstRect);

    status_t        getPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        getPreviewBdsSize(ExynosRect *dstRect);
    status_t        getPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        getPictureBdsSize(ExynosRect *dstRect);
    status_t        getFastenAeStableSensorSize(int *hwSensorW, int *hwSensorH);
    status_t        getFastenAeStableBcropSize(int *hwBcropW, int *hwBcropH);
    status_t        getFastenAeStableBdsSize(int *hwBdsW, int *hwBdsH);

    status_t        calcPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPreviewBDSSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcPictureBDSSize(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t        calcNormalToTpuSize(int srcW, int srcH, int *dstW, int *dstH);
    status_t        calcTpuToNormalSize(int srcW, int srcH, int *dstW, int *dstH);

private:
    /* Sets the dimensions for preview pictures. */
    void            m_setPreviewSize(int w, int h);
    /* Sets the image format for preview pictures. */
    void            m_setPreviewFormat(int colorFormat);
    /* Sets the dimensions for pictures. */
    void            m_setPictureSize(int w, int h);
    /* Sets the image format for pictures. */
    void            m_setPictureFormat(int colorFormat);
    /* Sets video's width, height */
    void            m_setVideoSize(int w, int h);
    /* Sets video's color format */
    void            m_setVideoFormat(int colorFormat);

    /* Sets the dimensions for Sesnor-related HW. */
    void            m_setHwSensorSize(int w, int h);
    /* Sets the dimensions for preview-related HW. */
    void            m_setHwPreviewSize(int w, int h);
    /* Sets the image format for preview-related HW. */
    void            m_setHwPreviewFormat(int colorFormat);
    /* Sets the dimensions for picture-related HW. */
    void            m_setHwPictureSize(int w, int h);
    /* Sets HW Bayer Crop Size */
    void            m_setHwBayerCropRegion(int w, int h, int x, int y);
    /* Sets the antibanding. */
    void            m_setAntibanding(int value);
    /* Sets the current color effect setting. */
    void            m_setColorEffectMode(aa_effect_mode_t effect);
    /* Sets the exposure compensation index. */
    void            m_setExposureCompensation(int value);
    /* Sets the flash mode. */
    void            m_setFlashMode(int flashMode);
    /* Sets focus areas. */
    void            m_setFocusAreas(uint32_t numValid, ExynosRect* rects, int *weights);
    /* Sets focus areas. (Using ExynosRect2) */
    void            m_setFocusAreas(uint32_t numValid, ExynosRect2* rect2s, int *weights);
    /* Sets the focus mode. */
    void            m_setFocusMode(int focusMode);
    /* Sets Jpeg quality of captured picture. */
    void            m_setJpegQuality(int quality);
    /* Sets the quality of the EXIF thumbnail in Jpeg picture. */
    void            m_setThumbnailQuality(int quality);
    /* Sets the dimensions for EXIF thumbnail in Jpeg picture. */
    void            m_setThumbnailSize(int w, int h);
    /* Sets metering areas. */
    void            m_setMeteringAreas(uint32_t num, ExynosRect  *rects, int *weights);
    /* Sets metering areas.(Using ExynosRect2) */
    void            m_setMeteringAreas(uint32_t num, ExynosRect2 *rect2s, int *weights);
    /* Sets the frame rate range for preview. */
    void            m_setPreviewFpsRange(uint32_t min, uint32_t max);
    /* Sets the scene mode. */
    void            m_setSceneMode(int sceneMode);
    /* Enables and disables video stabilization. */
    void            m_setVideoStabilization(bool stabilization);
    /* Sets the white balance. */
    status_t        m_setWhiteBalanceMode(int whiteBalance);
    /* Sets Bayer Crop Region */
    status_t            m_setParamCropRegion(int zoom, int srcW, int srcH, int dstW, int dstH);
    /* Sets recording mode hint. */
    void            m_setRecordingHint(bool hint);
    /* Sets GPS altitude. */
    void            m_setGpsAltitude(double altitude);
    /* Sets GPS latitude coordinate. */
    void            m_setGpsLatitude(double latitude);
    /* Sets GPS longitude coordinate. */
    void            m_setGpsLongitude(double longitude);
    /* Sets GPS processing method. */
    void            m_setGpsProcessingMethod(const char *gpsProcessingMethod);
    /* Sets GPS timestamp. */
    void            m_setGpsTimeStamp(long timeStamp);
    /* Sets the rotation angle in degrees relative to the orientation of the camera. */
    void            m_setRotation(int rotation);
    /* Sets the auto-exposure lock state. */
    void            m_setAutoExposureLock(bool lock);
    /* Sets the auto-white balance lock state. */
    void            m_setAutoWhiteBalanceLock(bool value);

/*
 * Additional API.
 */
    /* Sets metering areas. */
    void            m_setMeteringMode(int meteringMode);
    /* Sets brightness */
    void            m_setBrightness(int brightness);
    /* Sets ISO */
    void            m_setIso(uint32_t iso);
    /* Sets Contrast */
    void            m_setContrast(uint32_t contrast);
    /* Sets Saturation */
    void            m_setSaturation(int saturation);
    /* Sets Sharpness */
    void            m_setSharpness(int sharpness);
    /* Sets Hue */
    void            m_setHue(int hue);
    /* Sets WDR */
    void            m_setHdrMode(bool hdr);
    /* Sets anti shake */
    void            m_setAntiShake(bool toggle);
    /* Sets gamma */
    void            m_setGamma(bool gamma);
    /* Sets ODC */
    void            m_setOdcMode(bool toggle);
    /* Sets Slow AE */
    void            m_setSlowAe(bool slowAe);
    /* Sets 3DNR */
    void            m_set3dnrMode(bool toggle);
    /* Sets DRC */
    void            m_setDrcMode(bool toggle);

/*
 * Vendor specific APIs
 */
    /* Sets Intelligent mode */
    status_t        m_setIntelligentMode(int intelligentMode);
    void            m_setVisionMode(bool vision);
    void            m_setVisionModeFps(int fps);
    void            m_setVisionModeAeTarget(int ae);

    void            m_setSWVdisMode(bool swVdis);
    void            m_setSWVdisUIMode(bool swVdisUI);

    /* Sets VT mode */
    void            m_setVtMode(int vtMode);

    /* Sets Dual mode */
    void            m_setDualMode(bool toggle);
    /* Sets dual recording mode hint. */
    void            m_setDualRecordingHint(bool hint);
    /* Sets effect hint. */
    void            m_setEffectHint(bool hint);

    void            m_setHighResolutionCallbackMode(bool enable);
    void            m_setHighSpeedRecording(bool highSpeed);
    void            m_setCityId(long long int cityId);
    void            m_setWeatherId(unsigned char cityId);
    status_t        m_setImageUniqueId(const char *uniqueId);
    /* Sets camera angle */
    bool            m_setAngle(int angle);
    /* Sets Top-down mirror */
    bool            m_setTopDownMirror(void);
    /* Sets Left-right mirror */
    bool            m_setLRMirror(void);
    /* Sets Burst mode */
    void            m_setSeriesShotCount(int seriesShotCount);
    bool            m_setAutoFocusMacroPosition(int autoFocusMacroPosition);
    /* Sets Low Light A */
    bool            m_setLLAMode(void);

    /* Sets object tracking */
    bool            m_setObjectTracking(bool toggle);
    /* Start or stop object tracking operation */
    bool            m_setObjectTrackingStart(bool toggle);
    /* Sets x, y position for object tracking operation */
    bool            m_setObjectPosition(int x, int y);
    /* Sets smart auto */
    bool            m_setSmartAuto(bool toggle);
    /* Sets beauty shot */
    bool            m_setBeautyShot(bool toggle);

/*
 * Others
 */
    void            m_setRestartPreviewChecked(bool restart);
    bool            m_getRestartPreviewChecked(void);
    void            m_setRestartPreview(bool restart);
    void            m_setExifFixedAttribute(void);
    int             m_convertMetaCtlAwbMode(struct camera2_shot_ext *shot_ext);

public:

#ifdef SAMSUNG_DOF
    /* Sets Lens Position */
    void            m_setLensPosition(int step);
#endif
    /* Returns the dimensions setting for preview pictures. */
    void            getPreviewSize(int *w, int *h);
    /* Returns the image format for preview frames got from Camera.PreviewCallback. */
    int             getPreviewFormat(void);
    /* Returns the dimension setting for pictures. */
    void            getPictureSize(int *w, int *h);
    /* Returns the image format for pictures. */
    int             getPictureFormat(void);
    /* Gets video's width, height */
    void            getVideoSize(int *w, int *h);
    /* Gets video's color format */
    int             getVideoFormat(void);
    /* Gets the supported sensor sizes. */
    void            getMaxSensorSize(int *w, int *h);
    /* Gets the supported sensor margin. */
    void            getSensorMargin(int *w, int *h);
    /* Gets the supported preview sizes. */
    void            getMaxPreviewSize(int *w, int *h);
    /* Gets the supported picture sizes. */
    void            getMaxPictureSize(int *w, int *h);
    /* Gets the supported video frame sizes that can be used by MediaRecorder. */
    void            getMaxVideoSize(int *w, int *h);
    /* Gets the supported jpeg thumbnail sizes. */
    bool            getSupportedJpegThumbnailSizes(int *w, int *h);

    /* Returns the dimensions setting for preview-related HW. */
    void            getHwSensorSize(int *w, int *h);
    /* Returns the dimensions setting for preview-related HW. */
    void            getHwPreviewSize(int *w, int *h);
    /* Returns the image format for preview-related HW. */
    int             getHwPreviewFormat(void);
    /* Returns the dimension setting for picture-related HW. */
    void            getHwPictureSize(int *w, int *h);
    /* Returns HW Bayer Crop Size */
    void            getHwBayerCropRegion(int *w, int *h, int *x, int *y);

    /* Gets the current antibanding setting. */
    int             getAntibanding(void);
    /* Gets the state of the auto-exposure lock. */
    bool            getAutoExposureLock(void);
    /* Gets the state of the auto-white balance lock. */
    bool            getAutoWhiteBalanceLock(void);
    /* Gets the current color effect setting. */
    aa_effect_mode_t getColorEffectMode(void);
    /* Gets the current exposure compensation index. */
    int             getExposureCompensation(void);
    /* Gets the current flash mode setting. */
    int             getFlashMode(void);
    /* Gets the current focus areas. */
    void            getFocusAreas(int *validFocusArea, ExynosRect2 *rect2s, int *weights);
    /* Gets the current focus mode setting. */
    int             getFocusMode(void);
    /* Returns the quality setting for the JPEG picture. */
    int             getJpegQuality(void);
    /* Returns the quality setting for the EXIF thumbnail in Jpeg picture. */
    int             getThumbnailQuality(void);
    /* Returns the dimensions for EXIF thumbnail in Jpeg picture. */
    void            getThumbnailSize(int *w, int *h);
    /* Returns the max size for EXIF thumbnail in Jpeg picture. */
    void            getMaxThumbnailSize(int *w, int *h);
    /* Gets the current metering areas. */
    void            getMeteringAreas(ExynosRect *rects);
    /* Gets the current metering areas.(Using ExynosRect2) */
    void            getMeteringAreas(ExynosRect2 *rect2s);
    /* Returns the current minimum and maximum preview fps. */
    void            getPreviewFpsRange(uint32_t *min, uint32_t *max);
    /* Gets scene mode */
    int             getSceneMode(void);
    /* Gets the current state of video stabilization. */
    bool            getVideoStabilization(void);
    /* Gets the current white balance setting. */
    int             getWhiteBalanceMode(void);
    /* Sets current zoom value. */
    status_t        setZoomLevel(int value);
    /* Gets current zoom value. */
    int             getZoomLevel(void);
    /* Returns the recording mode hint. */
    bool            getRecordingHint(void);
    /* Gets GPS altitude. */
    double          getGpsAltitude(void);
    /* Gets GPS latitude coordinate. */
    double          getGpsLatitude(void);
    /* Gets GPS longitude coordinate. */
    double          getGpsLongitude(void);
    /* Gets GPS processing method. */
    const char *    getGpsProcessingMethod(void);
    /* Gets GPS timestamp. */
    long            getGpsTimeStamp(void);
    /* Gets the rotation angle in degrees relative to the orientation of the camera. */
    int             getRotation(void);

/*
 * Additional API.
 */

    /* Gets metering */
    int             getMeteringMode(void);
    /* Gets brightness */
    int             getBrightness(void);
    /* Gets ISO */
    uint32_t        getIso(void);
    /* Gets Contrast */
    uint32_t        getContrast(void);
    /* Gets Saturation */
    int             getSaturation(void);
    /* Gets Sharpness */
    int             getSharpness(void);
    /* Gets Hue */
    int             getHue(void);
    /* Gets WDR */
    bool            getHdrMode(void);
    /* Gets anti shake */
    bool            getAntiShake(void);
    /* Gets gamma */
    bool            getGamma(void);
    /* Gets ODC */
    bool            getOdcMode(void);
    /* Gets Slow AE */
    bool            getSlowAe(void);
    /* Gets Shot mode */
    int             getShotMode(void);
    /* Gets Preview Buffer Count */
    int             getPreviewBufferCount(void);
    /* Sets Preview Buffer Count */
    void            setPreviewBufferCount(int previewBufferCount);

    /* Gets 3DNR */
    bool            get3dnrMode(void);
    /* Gets DRC */
    bool            getDrcMode(void);
    /* Gets TPU enable case or not */
    bool            getTpuEnabledMode(void);

/*
 * Vendor specific APIs
 */

    /* Gets Intelligent mode */
    int             getIntelligentMode(void);
    bool            getVisionMode(void);
    int             getVisionModeFps(void);
    int             getVisionModeAeTarget(void);

    bool            isSWVdisMode(void); /* need to change name */
    bool            isSWVdisModeWithParam(int nPreviewW, int nPreviewH);
    bool            getSWVdisMode(void);
    bool            getSWVdisUIMode(void);

    bool            getHWVdisMode(void);
    int             getHWVdisFormat(void);

    /* Gets VT mode */
    int             getVtMode(void);

    /* Gets Dual mode */
    bool            getDualMode(void);
    /* Returns the dual recording mode hint. */
    bool            getDualRecordingHint(void);
    /* Returns the effect hint. */
    bool            getEffectHint(void);

    void            setFastFpsMode(int fpsMode);
    int             getFastFpsMode(void);

    bool            getHighResolutionCallbackMode(void);
    bool            getHighSpeedRecording(void);
    bool            getScalableSensorMode(void);
    void            setScalableSensorMode(bool scaleMode);
    long long int   getCityId(void);
    unsigned char   getWeatherId(void);
    /* Gets ImageUniqueId */
    const char     *getImageUniqueId(void);
    void            setImageUniqueId(char *uniqueId);
    /* Gets camera angle */
    int             getAngle(void);

    void            setFlipHorizontal(int val);
    int             getFlipHorizontal(void);
    void            setFlipVertical(int val);
    int             getFlipVertical(void);

    /* Gets Burst mode */
    int             getSeriesShotCount(void);
    /* Return callback need CSC */
    bool            getCallbackNeedCSC(void);
    /* Return callback need copy to rendering */
    bool            getCallbackNeedCopy2Rendering(void);

    /* Gets Illumination */
    int             getIllumination(void);
    /* Gets Low Light Shot */
    int             getLLS(struct camera2_shot_ext *shot);
    /* Gets Low Light A */
    bool            getLLAMode(void);
    /* Sets the device orientation angle in degrees to camera FW for FD scanning property. */
    bool            setDeviceOrientation(int orientation);
    /* Gets the device orientation angle in degrees . */
    int             getDeviceOrientation(void);
    /* Gets the FD orientation angle in degrees . */
    int             getFdOrientation(void);
    /* Gets object tracking */
    bool            getObjectTracking(void);
    /* Gets status of object tracking operation */
    int             getObjectTrackingStatus(void);
    /* Gets smart auto */
    bool            getSmartAuto(void);
    /* Gets the status of smart auto operation */
    int             getSmartAutoStatus(void);
    /* Gets beauty shot */
    bool            getBeautyShot(void);

/*
 * Static info
 */
    /* Gets the exposure compensation step. */
    float           getExposureCompensationStep(void);

    /* Gets the focal length (in millimeter) of the camera. */
    void            getFocalLength(int *num, int *den);

    /* Gets the distances from the camera to where an object appears to be in focus. */
    void            getFocusDistances(int *num, int *den);;

    /* Gets the minimum exposure compensation index. */
    int             getMinExposureCompensation(void);

    /* Gets the maximum exposure compensation index. */
    int             getMaxExposureCompensation(void);

    /* Gets the maximum number of detected faces supported. */
    int             getMaxNumDetectedFaces(void);

    /* Gets the maximum number of focus areas supported. */
    uint32_t        getMaxNumFocusAreas(void);

    /* Gets the maximum number of metering areas supported. */
    uint32_t        getMaxNumMeteringAreas(void);

    /* Gets the maximum zoom value allowed for snapshot. */
    int             getMaxZoomLevel(void);

    /* Gets the supported antibanding values. */
    int             getSupportedAntibanding(void);

    /* Gets the supported color effects. */
    int             getSupportedColorEffects(void);

    /* Gets the supported color effects & hidden color effect. */
    bool            isSupportedColorEffects(aa_effect_mode_t effectMode);

    /* Check whether the target support Flash */
    int             getSupportedFlashModes(void);

    /* Gets the supported focus modes. */
    int             getSupportedFocusModes(void);

    /* Gets the supported preview fps range. */
    bool            getMaxPreviewFpsRange(int *min, int *max);

    /* Gets the supported scene modes. */
    int             getSupportedSceneModes(void);

    /* Gets the supported white balance. */
    int             getSupportedWhiteBalance(void);

    /* Gets the supported Iso values. */
    int             getSupportedISO(void);

    /* Gets max zoom ratio */
    float           getMaxZoomRatio(void);
    /* Gets zoom ratio */
    float           getZoomRatio(int zoom);

    /* Returns true if auto-exposure locking is supported. */
    bool            getAutoExposureLockSupported(void);

    /* Returns true if auto-white balance locking is supported. */
    bool            getAutoWhiteBalanceLockSupported(void);

    /* Returns true if smooth zoom is supported. */
    bool            getSmoothZoomSupported(void);

    /* Returns true if video snapshot is supported. */
    bool            getVideoSnapshotSupported(void);

    /* Returns true if video stabilization is supported. */
    bool            getVideoStabilizationSupported(void);

    /* Returns true if zoom is supported. */
    bool            getZoomSupported(void);

    /* Gets the horizontal angle of view in degrees. */
    void            checkHorizontalViewAngle(void);
    float           getHorizontalViewAngle(void);

    /* Gets the vertical angle of view in degrees. */
    float           getVerticalViewAngle(void);

    /* Gets Fnumber */
    void            getFnumber(int *num, int *den);

    /* Gets Aperture value */
    void            getApertureValue(int *num, int *den);

    /* Gets FocalLengthIn35mmFilm */
    int             getFocalLengthIn35mmFilm(void);

    bool            isScalableSensorSupported(void);

    status_t        getFixedExifInfo(exif_attribute_t *exifInfo);
    void            setExifChangedAttribute(exif_attribute_t    *exifInfo,
                                            ExynosRect          *PictureRect,
                                            ExynosRect          *thumbnailRect,
                                            camera2_shot_t      *shot);

    debug_attribute_t *getDebugAttribute(void);

#ifdef DEBUG_RAWDUMP
    bool           checkBayerDumpEnable(void);
#endif/* DEBUG_RAWDUMP */
#ifdef USE_BINNING_MODE
    status_t        needBinningMode(void);
#endif /* USE_BINNING_MODE */
public:
    bool            DvfsLock();
    bool            DvfsUnLock();

    void            updatePreviewFpsRange(void);
    void            updateHwSensorSize(void);
    void            updateBinningScaleRatio(void);
    void            updateBnsScaleRatio(void);

    void            setHwPreviewStride(int stride);
    int             getHwPreviewStride(void);

    status_t        duplicateCtrlMetadata(void *buf);

    status_t        setRequestDis(int value);

    status_t        setDisEnable(bool enable);
    status_t        setDrcEnable(bool enable);
    status_t        setDnrEnable(bool enable);
    status_t        setFdEnable(bool enable);

    bool            getDisEnable(void);
    bool            getDrcEnable(void);
    bool            getDnrEnable(void);
    bool            getFdEnable(void);

    status_t        setFdMode(enum facedetect_mode mode);
    status_t        getFdMeta(bool reprocessing, void *buf);
    bool            getUHDRecordingMode(void);

private:
    bool            m_isSupportedPreviewSize(const int width, const int height);
    bool            m_isSupportedPictureSize(const int width, const int height);
    bool            m_isSupportedVideoSize(const int width, const int height);
    bool            m_isHighResolutionCallbackSize(const int width, const int height);
    void            m_isHighResolutionMode(const CameraParameters& params);

    bool            m_getSupportedVariableFpsList(int min, int max,
                                                int *newMin, int *newMax);

    status_t        m_getPreviewSizeList(int *sizeList);

    void            m_getSWVdisPreviewSize(int w, int h, int *newW, int *newH);
    void            m_getScalableSensorSize(int *newSensorW, int *newSensorH);

    void            m_initMetadata(void);

    bool            m_isUHDRecordingMode(void);

/*
 * Vendor specific adjust function
 */
private:
    status_t        m_adjustPreviewFpsRange(int &newMinFps, int &newMaxFps);
    status_t        m_getPreviewBdsSize(ExynosRect *dstRect);
    status_t        m_adjustPreviewSize(int previewW, int previewH,
                                        int *newPreviewW, int *newPreviewH,
                                        int *newCalPreviewW, int *newCalPreviewH);
    status_t        m_adjustPreviewFormat(int &previewFormat, int &hwPreviewFormatH);
    status_t        m_adjustPictureSize(int *newPictureW, int *newPictureH,
                                        int *newHwPictureW, int *newHwPictureH);
    bool            m_adjustHighSpeedRecording(int curMinFps, int curMaxFps, int newMinFps, int newMaxFps);
    const char *    m_adjustAntibanding(const char *strAntibanding);
    const char *    m_adjustFocusMode(const char *focusMode);
    const char *    m_adjustFlashMode(const char *flashMode);
    const char *    m_adjustWhiteBalanceMode(const char *whiteBalance);
    bool            m_adjustScalableSensorMode(const int scaleMode);
    void            m_adjustSensorMargin(int *sensorMarginW, int *sensorMarginH);
    void            m_getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange);

    /* for initial 120fps start due to quick launch */
/*
    void            set120FpsState(enum INIT_120FPS_STATE state);
    void            clear120FpsState(enum INIT_120FPS_STATE state);
    bool            flag120FpsStart(void);
    bool            setSensorFpsAfter120fps(void);
    void            setInitValueAfter120fps(bool isAfter);
*/

    status_t        m_setBinningScaleRatio(int ratio);
    status_t        m_setBnsScaleRatio(int ratio);
    status_t        m_addHiddenResolutionList(String8 &string8Buf, struct ExynosSensorInfo *sensorInfo,
                                              int w, int h, enum MODE mode, int cameraId);
    void            m_setExifChangedAttribute(exif_attribute_t  *exifInfo,
                                              ExynosRect        *PictureRect,
                                              ExynosRect        *thumbnailRect,
                                              camera2_dm        *dm,
                                              camera2_udm       *udm);

public:
    status_t            setParameters(const CameraParameters& params);
    CameraParameters    getParameters() const;
    int                 getCameraId(void);
    /* Gets the detected faces areas. */
    int                 getDetectedFacesAreas(int num, int *id,
                                              int *score, ExynosRect *face,
                                              ExynosRect *leftEye, ExynosRect *rightEye,
                                              ExynosRect *mouth);
    /* Gets the detected faces areas. (Using ExynosRect2) */
    int                 getDetectedFacesAreas(int num, int *id,
                                              int *score, ExynosRect2 *face,
                                              ExynosRect2 *leftEye, ExynosRect2 *rightEye,
                                              ExynosRect2 *mouth);

    void                enableMsgType(int32_t msgType);
    void                disableMsgType(int32_t msgType);
    bool                msgTypeEnabled(int32_t msgType);

    status_t            setFrameSkipCount(int count);
    status_t            getFrameSkipCount(int *count);
    int                 getFrameSkipCount(void);

    void                setIsFirstStartFlag(bool flag);
    int                 getIsFirstStartFlag(void);

    void                setPreviewRunning(bool enable);
    void                setPictureRunning(bool enable);
    void                setRecordingRunning(bool enable);
    bool                getPreviewRunning(void);
    bool                getPictureRunning(void);
    bool                getRecordingRunning(void);
    bool                getRestartPreview(void);

    ExynosCameraActivityControl *getActivityControl(void);
    status_t            setAutoFocusMacroPosition(int autoFocusMacroPosition);

    void                getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange);
    void                setSetfileYuvRange(void);
    status_t            checkSetfileYuvRange(void);

    void                setUseDynamicBayer(bool enable);
    bool                getUseDynamicBayer(void);
    void                setUseDynamicBayerVideoSnapShot(bool enable);
    bool                getUseDynamicBayerVideoSnapShot(void);
    void                setUseDynamicScc(bool enable);
    bool                getUseDynamicScc(void);

    void                setUseFastenAeStable(bool enable);
    bool                getUseFastenAeStable(void);

    void                setUsePureBayerReprocessing(bool enable);
    bool                getUsePureBayerReprocessing(void);

    int32_t             getReprocessingBayerMode(void);

    void                setAdaptiveCSCRecording(bool enable);
    bool                getAdaptiveCSCRecording(void);
    bool                doCscRecording(void);

    uint32_t            getBinningScaleRatio(void);
    uint32_t            getBnsScaleRatio(void);
    /* Sets the dimensions for Sesnor-related BNS. */
    void                setBnsSize(int w, int h);
    /* Gets the dimensions for Sesnor-related BNS. */
    void                getBnsSize(int *w, int *h);

    /*
     * This must call before startPreview(),
     * this update h/w setting at once.
     */
    bool                updateTpuParameters(void);
    int                 getHalVersion(void);
    void                setHalVersion(int halVersion);
    struct ExynosSensorInfo *getSensorStaticInfo();

#ifdef SAMSUNG_DOF
    int                 getMoveLensTotal(void);
    void                setMoveLensCount(int count);
    void                setMoveLensTotal(int count);
#endif

#ifdef BURST_CAPTURE
    int                 getSeriesShotSaveLocation(void);
    void                setSeriesShotSaveLocation(int ssaveLocation);
    char                *getSeriesShotFilePath(void);
#endif
    int                 getSeriesShotDuration(void);
    int                 getSeriesShotMode(void);
    void                setSeriesShotMode(int sshotMode, int count = 0);

    int                 getHalPixelFormat(void);
    int                 convertingHalPreviewFormat(int previewFormat, int yuvRange);

    void                setDvfsLock(bool lock);
    bool                getDvfsLock(void);

    void                setFocusModeSetting(bool enable);
    int                 getFocusModeSetting(void);
    bool                getSensorOTFSupported(void);
    bool                isReprocessing(void);
    bool                isSccCapture(void);
    bool                isFlite3aaOtf(void);
    bool                is3aaIspOtf(void);
    bool                isReprocessing3aaIspOTF(void);
    bool                isUsing3acForIspc(void);

    bool                getReallocBuffer();
    bool                setReallocBuffer(bool enable);

    bool                setConfig(struct ExynosConfigInfo* config);
    struct ExynosConfigInfo* getConfig();

    bool                setConfigMode(uint32_t mode);
    int                 getConfigMode();
    /* Sets Shot mode */
    void                m_setShotMode(int shotMode);
    void                setZoomActiveOn(bool enable);
    bool                getZoomActiveOn(void);
    void                setFocusModeLock(bool enable);
    status_t	        setMarkingOfExifFlash(int flag);
    int		            getMarkingOfExifFlash(void);
    bool                increaseMaxBufferOfPreview(void);

//Added
    int                 getHDRDelay(void) { return HDR_DELAY; }
    int                 getReprocessingBayerHoldCount(void) { return REPROCESSING_BAYER_HOLD_COUNT; }
    int                 getFastenAeFps(void) { return FASTEN_AE_FPS; }
    int                 getPerFrameControlPipe(void) {return PERFRAME_CONTROL_PIPE; }
    int                 getPerFrameControlReprocessingPipe(void) {return PERFRAME_CONTROL_REPROCESSING_PIPE; }
    int                 getPerFrameInfo3AA(void) { return PERFRAME_INFO_3AA; };
    int                 getPerFrameInfoIsp(void) { return PERFRAME_INFO_ISP; };
    int                 getPerFrameInfoDis(void) { return PERFRAME_INFO_DIS; };
    int                 getPerFrameInfoReprocessingPure3AA(void) { return PERFRAME_INFO_PURE_REPROCESSING_3AA; }
    int                 getPerFrameInfoReprocessingPureIsp(void) { return PERFRAME_INFO_PURE_REPROCESSING_ISP; }
    int                 getScalerNodeNumPicture(void) { return PICTURE_GSC_NODE_NUM;}
    int                 getScalerNodeNumPreview(void) { return PREVIEW_GSC_NODE_NUM;}
    int                 getScalerNodeNumVideo(void) { return VIDEO_GSC_NODE_NUM;}
    bool                isOwnScc(int cameraId);
    bool                needGSCForCapture(int camId) { return (camId == CAMERA_ID_BACK) ? USE_GSC_FOR_CAPTURE_BACK : USE_GSC_FOR_CAPTURE_FRONT; }
    bool                getSetFileCtlMode(void);
    bool                getSetFileCtl3AA_ISP(void);
    bool                getSetFileCtl3AA(void);
    bool                getSetFileCtlISP(void);
    bool                getSetFileCtlSCP(void);

private:
    int                         m_cameraId;
    char                        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    CameraParameters            m_params;
    struct camera2_shot_ext     m_metadata;

    struct exynos_camera_info   m_cameraInfo;
    struct ExynosSensorInfo    *m_staticInfo;

    exif_attribute_t            m_exifInfo;
    debug_attribute_t           mDebugInfo;

    int32_t                     m_enabledMsgType;
    mutable Mutex               m_msgLock;

    /* frame skips */
    ExynosCameraCounter         m_frameSkipCounter;

    mutable Mutex               m_parameterLock;

    ExynosCameraActivityControl *m_activityControl;

    /* Flags for camera status */
    bool                        m_previewRunning;
    bool                        m_previewSizeChanged;
    bool                        m_pictureRunning;
    bool                        m_recordingRunning;
    bool                        m_flagCheckDualMode;
    bool                        m_flagRestartPreviewChecked;
    bool                        m_flagRestartPreview;
    int                         m_fastFpsMode;
    bool                        m_flagFirstStart;
    bool                        m_flagMeteringRegionChanged;
    bool                        m_flagHWVDisMode;

    bool                        m_flagVideoStabilization;
    bool                        m_flag3dnrMode;

    bool                        m_flagCheckRecordingHint;

    int                         m_setfile;
    int                         m_yuvRange;
    int                         m_setfileReprocessing;
    int                         m_yuvRangeReprocessing;

    bool                        m_useSizeTable;
    bool                        m_useDynamicBayer;
    bool                        m_useDynamicBayerVideoSnapShot;
    bool                        m_useDynamicScc;
    bool                        m_useFastenAeStable;
    bool                        m_usePureBayerReprocessing;
    bool                        m_useAdaptiveCSCRecording;
    bool                        m_dvfsLock;
    int                         m_previewBufferCount;

    bool                        m_reallocBuffer;
    mutable Mutex               m_reallocLock;

    struct ExynosConfigInfo     *m_exynosconfig;

    int                         m_curLensStep;
    int                         m_curLensCount;
    bool                        m_setFocusmodeSetting;
    bool                        m_zoom_activated;
    int                         m_firing_flash_marking;

    int                         m_halVersion;
};
}; /* namespace android */
#endif

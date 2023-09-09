/*
**
** Copyright 2013, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_HW_IMPLEMENTATION_H
#define EXYNOS_CAMERA_HW_IMPLEMENTATION_H

#include <media/hardware/HardwareAPI.h>

#include "ExynosCameraDefine.h"

#ifdef SAMSUNG_TN_FEATURE
#include "SecCameraParameters.h"
#endif
#ifdef SAMSUNG_UNIPLUGIN
#include "uni_plugin_wrapper.h"
#endif

#ifdef SAMSUNG_SENSOR_LISTENER
#include "sensor_listener_wrapper.h"
#endif

namespace android {

class ExynosCamera {
public:
    ExynosCamera() {};
    ExynosCamera(int cameraId, camera_device_t *dev);
    virtual             ~ExynosCamera();

    status_t    setPreviewWindow(preview_stream_ops *w);
    void        setCallbacks(
                    camera_notify_callback notify_cb,
                    camera_data_callback data_cb,
                    camera_data_timestamp_callback data_cb_timestamp,
                    camera_request_memory get_memory,
                    void *user);

    void        enableMsgType(int32_t msgType);
    void        disableMsgType(int32_t msgType);
    bool        msgTypeEnabled(int32_t msgType);

    status_t    startPreview();
    void        stopPreview();
    bool        previewEnabled();

    status_t    storeMetaDataInBuffers(bool enable);

    status_t    startRecording();
    void        stopRecording();
    bool        recordingEnabled();
    void        releaseRecordingFrame(const void *opaque);

    status_t    autoFocus();
    status_t    cancelAutoFocus();

    status_t    takePicture();
    status_t    cancelPicture();

    status_t    setParameters(const CameraParameters& params);
    CameraParameters  getParameters() const;
    status_t    sendCommand(int32_t command, int32_t arg1, int32_t arg2);

    int         getMaxNumDetectedFaces(void);
    bool        startFaceDetection(void);
    bool        stopFaceDetection(void);

#ifdef SAMSUNG_DOF
    bool        startCurrentSet(void);
    bool        stopCurrentSet(void);
#endif

    bool        m_startFaceDetection(bool toggle);
    int         m_calibratePosition(int w, int new_w, int pos);
    status_t    m_doFdCallbackFunc(ExynosCameraFrame *frame);
#ifdef SR_CAPTURE
    status_t    m_doSRCallbackFunc(ExynosCameraFrame *frame);
#endif
    void        release();

    status_t    dump(int fd) const;
    void        dump(void);

    int         getCameraId() const;
    int         getShotBufferIdex() const;
    status_t    generateFrame(int32_t frameCount, ExynosCameraFrame **newFrame);
    status_t    generateFrameSccScp(uint32_t pipeId, uint32_t *frameCount, ExynosCameraFrame **newFrame);

    status_t    generateFrameReprocessing(ExynosCameraFrame **newFrame);

    /* vision */
    status_t    generateFrameVision(int32_t frameCount, ExynosCameraFrame **newFrame);

private:
    void        m_createThreads(void);

    status_t    m_startPreviewInternal(void);
    status_t    m_stopPreviewInternal(void);

    status_t    m_restartPreviewInternal(void);

    status_t    m_startPictureInternal(void);
    status_t    m_stopPictureInternal(void);

    status_t    m_startRecordingInternal(void);
    status_t    m_stopRecordingInternal(void);

    status_t    m_searchFrameFromList(List<ExynosCameraFrame *> *list, uint32_t frameCount, ExynosCameraFrame **frame);
    status_t    m_removeFrameFromList(List<ExynosCameraFrame *> *list, ExynosCameraFrame *frame);
    status_t    m_deleteFrame(ExynosCameraFrame **frame);

    status_t    m_clearList(List<ExynosCameraFrame *> *list);
    status_t    m_clearList(frame_queue_t *queue);
    status_t    m_clearFrameQ(frame_queue_t *frameQ, uint32_t pipeId, uint32_t direction);

    status_t    m_printFrameList(List<ExynosCameraFrame *> *list);

    status_t    m_createIonAllocator(ExynosCameraIonAllocator **allocator);
    status_t    m_createInternalBufferManager(ExynosCameraBufferManager **bufferManager, const char *name);
    status_t    m_createBufferManager(
                    ExynosCameraBufferManager **bufferManager,
                    const char *name,
                    buffer_manager_type type = BUFFER_MANAGER_ION_TYPE);

    status_t    m_setFrameManager();

    status_t    m_setConfigInform();
    status_t    m_setBuffers(void);
    status_t    m_setReprocessingBuffer(void);
    status_t    m_setPreviewCallbackBuffer(void);
    status_t    m_setPictureBuffer(void);
    status_t    m_releaseBuffers(void);

    status_t    m_putBuffers(ExynosCameraBufferManager *bufManager, int bufIndex);
    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  reqBufCount,
                    bool createMetaPlane,
                    bool needMmap = false);
    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  minBufCount,
                    int  maxBufCount,
                    exynos_camera_buffer_type_t type,
                    bool createMetaPlane,
                    bool needMmap = false);
    status_t    m_allocBuffers(
                    ExynosCameraBufferManager *bufManager,
                    int  planeCount,
                    unsigned int *planeSize,
                    unsigned int *bytePerLine,
                    int  minBufCount,
                    int  maxBufCount,
                    exynos_camera_buffer_type_t type,
                    buffer_manager_allocation_mode_t allocMode,
                    bool createMetaPlane,
                    bool needMmap = false);

    status_t    m_handlePreviewFrame(ExynosCameraFrame *frame);
    status_t    m_handlePreviewFrameFront(ExynosCameraFrame *frame);
    status_t    m_handlePreviewFrameFrontDual(ExynosCameraFrame *frame);

    status_t    m_setupEntity(
                    uint32_t pipeId,
                    ExynosCameraFrame *newFrame,
                    ExynosCameraBuffer *srcBuf = NULL,
                    ExynosCameraBuffer *dstBuf = NULL);
    status_t    m_setSrcBuffer(
                    uint32_t pipeId,
                    ExynosCameraFrame *newFrame,
                    ExynosCameraBuffer *buffer);
    status_t    m_setDstBuffer(
                    uint32_t pipeId,
                    ExynosCameraFrame *newFrame,
                    ExynosCameraBuffer *buffer);

    status_t    m_getBufferManager(uint32_t pipeId, ExynosCameraBufferManager **bufMgr, uint32_t direction);

    status_t    m_calcPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t    m_calcHighResolutionPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t    m_calcRecordingGSCRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t    m_calcPictureRect(ExynosRect *srcRect, ExynosRect *dstRect);
    status_t    m_calcPictureRect(int originW, int originH, ExynosRect *srcRect, ExynosRect *dstRect);

    status_t    m_setCallbackBufferInfo(ExynosCameraBuffer *callbackBuf, char *baseAddr);

    status_t    m_doPreviewToCallbackFunc(
                    int32_t pipeId,
                    ExynosCameraFrame *newFrame,
                    ExynosCameraBuffer previewBuf,
                    ExynosCameraBuffer callbackBuf);
    status_t    m_doCallbackToPreviewFunc(
                    int32_t pipeId,
                    ExynosCameraFrame *newFrame,
                    ExynosCameraBuffer callbackBuf,
                    ExynosCameraBuffer previewBuf);
    status_t    m_doPrviewToRecordingFunc(
                    int32_t pipeId,
                    ExynosCameraBuffer previewBuf,
                    ExynosCameraBuffer recordingBuf,
                    nsecs_t timeStamp);
    status_t    m_syncPrviewWithCSC(int32_t pipeId, int32_t gscPipe, ExynosCameraFrame *frame);
    status_t    m_releaseRecordingBuffer(int bufIndex);

    status_t    m_fastenAeStable(void);
    camera_memory_t *m_getJpegCallbackHeap(ExynosCameraBuffer callbackBuf, int seriesShotNumber);

    void        m_debugFpsCheck(uint32_t pipeId);

    uint32_t    m_getBayerPipeId(void);

    status_t    m_convertingStreamToShotExt(ExynosCameraBuffer *buffer, struct camera2_node_output *outputInfo);

    status_t    m_getBayerBuffer(uint32_t pipeId, ExynosCameraBuffer *buffer, camera2_shot_ext *updateDmShot = NULL);
    status_t    m_checkBufferAvailable(uint32_t pipeId, ExynosCameraBufferManager *bufferMgr);

    status_t    m_boostDynamicCapture(void);
    void        m_updateBoostDynamicCaptureSize(camera2_node_group *node_group_info);
    void        m_checkFpsAndUpdatePipeWaitTime(void);
    void        m_printExynosCameraInfo(const char *funcName);

    status_t    m_setupFrameFactory(void);
    status_t    m_initFrameFactory(void);
    status_t    m_deinitFrameFactory(void);
#ifdef ONE_SECOND_BURST_CAPTURE
    void        m_clearOneSecondBurst(bool isJpegCallbackThread);
#endif
    void        m_checkEntranceLux(struct camera2_shot_ext *meta_shot_ext);
    status_t    m_copyMetaFrameToFrame(ExynosCameraFrame *srcframe, ExynosCameraFrame *dstframe, bool useDm, bool useUdm);

#ifdef SAMSUNG_DEBLUR
    void        setUseDeblurCaptureOn(bool enable);
    bool        getUseDeblurCaptureOn(void);
    bool        m_deblurCaptureThreadFunc(void);
    bool        m_detectDeblurCaptureThreadFunc(void);
#endif

public:

    ExynosCameraFrameFactory        *m_frameFactory[FRAME_FACTORY_TYPE_MAX];

    ExynosCameraParameters          *m_exynosCameraParameters;
    ExynosCameraFrameFactory        *m_previewFrameFactory;
    ExynosCameraGrallocAllocator    *m_grAllocator;
    ExynosCameraIonAllocator        *m_ionAllocator;
    ExynosCameraMHBAllocator        *m_mhbAllocator;

    ExynosCameraFrameFactory        *m_pictureFrameFactory;

    ExynosCameraFrameFactory        *m_reprocessingFrameFactory;
    mutable Mutex                   m_frameLock;
    mutable Mutex                   m_searchframeLock;

    preview_stream_ops              *m_previewWindow;
    bool                            m_previewEnabled;
    bool                            m_pictureEnabled;
    bool                            m_recordingEnabled;
    bool                            m_zslPictureEnabled;
#ifdef OIS_CAPTURE
    bool                            m_OISCaptureShutterEnabled;
#endif
    bool                            m_use_companion;
    bool                            m_checkFirstFrameLux;
    ExynosCameraActivityControl     *m_exynosCameraActivityControl;

    /* vision */
    ExynosCameraFrameFactory        *m_visionFrameFactory;

private:
    typedef ExynosCameraThread<ExynosCamera> mainCameraThread;

    uint32_t                        m_cameraId;
    uint32_t                        m_cameraSensorId;
    char                            m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    camera_notify_callback          m_notifyCb;
    camera_data_callback            m_dataCb;
    camera_data_timestamp_callback  m_dataCbTimestamp;
    camera_request_memory           m_getMemoryCb;
    void                            *m_callbackCookie;

    List<ExynosCameraFrame *>       m_processList;
    List<ExynosCameraFrame *>       m_postProcessList;
    List<ExynosCameraFrame *>       m_recordingProcessList;
    frame_queue_t                   *m_pipeFrameDoneQ;

    framefactory_queue_t            *m_frameFactoryQ;
    sp<mainCameraThread>            m_framefactoryThread;
    bool                            m_frameFactoryInitThreadFunc(void);

    /* vision */
    frame_queue_t                   *m_pipeFrameVisionDoneQ;

    sp<mainCameraThread>            m_mainThread;
    bool                            m_mainThreadFunc(void);

#ifdef SAMSUNG_COMPANION
    sp<mainCameraThread>            m_companionThread;
    bool                            m_companionThreadFunc(void);
    int                             m_getSensorId(int m_cameraId);
    ExynosCameraNode                *m_companionNode;
#endif
#ifdef SAMSUNG_EEPROM
    sp<mainCameraThread>            m_eepromThread;
    bool                            m_eepromThreadFunc(void);
#endif
#ifdef SAMSUNG_HLV
    status_t                        m_ProgramAndProcessHLV(ExynosCameraBuffer *FrameBuffer);
#endif
#ifdef SAMSUNG_JQ
    int                             m_processJpegQtable(ExynosCameraBuffer* buffer);
#endif

    frame_queue_t                   *m_previewQ;
    frame_queue_t                   *m_previewCallbackGscFrameDoneQ;
    sp<mainCameraThread>            m_previewThread;
    frame_queue_t                   *m_mainSetupQ[MAX_NUM_PIPES];
    sp<mainCameraThread>            m_mainSetupQThread[MAX_NUM_PIPES];
    bool                            m_previewThreadFunc(void);

    sp<mainCameraThread>            m_setBuffersThread;
    bool                            m_setBuffersThreadFunc(void);

    sp<mainCameraThread>            m_startPictureInternalThread;
    bool                            m_startPictureInternalThreadFunc(void);

    sp<mainCameraThread>            m_startPictureBufferThread;
    bool                            m_startPictureBufferThreadFunc(void);

    sp<mainCameraThread>            m_autoFocusThread;
    bool                            m_autoFocusThreadFunc(void);
    bool                            m_autoFocusResetNotify(int focusMode);
    mutable Mutex                   m_autoFocusLock;
    mutable Mutex                   m_captureLock;
    mutable Mutex                   m_recordingListLock;
    mutable Mutex                   m_recordingStopLock;
    bool                            m_exitAutoFocusThread;
    bool                            m_autoFocusRunning;
    int                             m_autoFocusType;

    typedef ExynosCameraList<uint32_t> worker_queue_t;
    worker_queue_t                  m_autoFocusContinousQ;
    sp<mainCameraThread>            m_autoFocusContinousThread;
    bool                            m_autoFocusContinousThreadFunc(void);

    ExynosCameraBufferManager       *m_bayerBufferMgr;
    ExynosCameraBufferManager       *m_3aaBufferMgr;
    ExynosCameraBufferManager       *m_ispBufferMgr;
    ExynosCameraBufferManager       *m_hwDisBufferMgr;
    ExynosCameraBufferManager       *m_sccBufferMgr;
    ExynosCameraBufferManager       *m_scpBufferMgr;
    ExynosCameraBufferManager       *m_zoomScalerBufferMgr;
#ifdef SAMSUNG_DNG
    ExynosCameraBufferManager       *m_fliteBufferMgr;
#endif

    uint32_t                        m_fliteFrameCount;
    uint32_t                        m_3aa_ispFrameCount;
    uint32_t                        m_ispFrameCount;
    uint32_t                        m_sccFrameCount;
    uint32_t                        m_scpFrameCount;

    int                             m_frameSkipCount;

    ExynosCameraFrameManager        *m_frameMgr;

    bool                            m_isSuccessedBufferAllocation;

    /* for Recording */
    bool                            m_doCscRecording;
    int                             m_recordingBufferCount;
    frame_queue_t                   *m_recordingQ;
    nsecs_t                         m_lastRecordingTimeStamp;
    nsecs_t                         m_recordingStartTimeStamp;

    ExynosCameraBufferManager       *m_recordingBufferMgr;
    camera_memory_t                 *m_recordingCallbackHeap;

    bool                            m_recordingBufAvailable[MAX_BUFFERS];
    nsecs_t                         m_recordingTimeStamp[MAX_BUFFERS];
    sp<mainCameraThread>            m_recordingThread;
    bool                            m_recordingThreadFunc(void);

    mutable Mutex                   m_recordingStateLock;
    bool                            m_getRecordingEnabled(void);
    void                            m_setRecordingEnabled(bool enable);

    ExynosCameraBufferManager       *m_previewCallbackBufferMgr;
    ExynosCameraBufferManager       *m_highResolutionCallbackBufferMgr;

    /* Pre picture Thread */
    sp<mainCameraThread>            m_prePictureThread;
    bool                            m_reprocessingPrePictureInternal(void);
    bool                            m_prePictureInternal(bool* pIsProcessed);
    bool                            m_prePictureThreadFunc(void);

    sp<mainCameraThread>            m_pictureThread;
    bool                            m_pictureThreadFunc(void);

    sp<mainCameraThread>            m_postPictureThread;
    bool                            m_postPictureThreadFunc(void);

    sp<mainCameraThread>            m_jpegCallbackThread;
    bool                            m_jpegCallbackThreadFunc(void);
    void                            m_clearJpegCallbackThread(bool callFromJpeg);

    bool                            m_releasebuffersForRealloc(void);
    bool                            m_CheckCameraSavingPath(char *dir, char* srcPath, char *dstPath, int dstSize);
    bool                            m_FileSaveFunc(char *filePath, ExynosCameraBuffer *SaveBuf);

    /* Reprocessing Buffer Managers */
    ExynosCameraBufferManager       *m_ispReprocessingBufferMgr;

    ExynosCameraBufferManager       *m_sccReprocessingBufferMgr;

    /* TODO: will be removed when SCC scaling for picture size */
    ExynosCameraBufferManager       *m_gscBufferMgr;

    ExynosCameraBufferManager       *m_postPictureGscBufferMgr;

#ifdef SAMSUNG_LBP
    ExynosCameraBufferManager       *m_lbpBufferMgr;
#endif
    ExynosCameraBufferManager       *m_jpegBufferMgr;

    ExynosCameraCounter             m_takePictureCounter;
    ExynosCameraCounter             m_reprocessingCounter;
    ExynosCameraCounter             m_pictureCounter;
    ExynosCameraCounter             m_jpegCounter;
#ifdef ONE_SECOND_BURST_CAPTURE
    ExynosCameraCounter             m_jpegCallbackCounter;
#endif

    /* Reprocessing Q */
    frame_queue_t                   *dstIspReprocessingQ;
    frame_queue_t                   *dstSccReprocessingQ;
    frame_queue_t                   *dstGscReprocessingQ;

    frame_queue_t                   *dstPostPictureGscQ;

#ifdef RAWDUMP_CAPTURE
    frame_queue_t                   *m_RawCaptureDumpQ;
    sp<mainCameraThread>            m_RawCaptureDumpThread;
    bool                            m_RawCaptureDumpThreadFunc(void);
#endif

#ifdef SAMSUNG_DNG
    dng_capture_queue_t             *m_dngCaptureQ;
    sp<mainCameraThread>            m_DNGCaptureThread;
    bool                            m_searchDNGFrame;
    unsigned int                    getDNGFcount();
    bool                            m_DNGCaptureThreadFunc(void);
    SecCameraDngCreator             m_dngCreator;
    unsigned int                    m_dngFrameNumber;
    char                            m_dngTime[20];
    char                            m_dngSavePath[CAMERA_FILE_PATH_SIZE];
    camera_memory_t*                m_getDngCallbackHeap(ExynosCameraBuffer *dngBuf);
    unsigned int                    m_dngFrameNumberForManualExposure;
    bool                            m_longExposureEnds;
    unsigned int                    m_dngLongExposureRemainCount;
#endif
    unsigned int                    m_longExposureRemainCount;
    bool                            m_longExposurePreview;
    bool                            m_stopLongExposure;
    bool                            m_cancelPicture;
    frame_queue_t                   *dstJpegReprocessingQ;

    frame_queue_t                   *m_postPictureQ;
    jpeg_callback_queue_t           *m_jpegCallbackQ;
    postview_callback_queue_t       *m_postviewCallbackQ;
    thumbnail_callback_queue_t      *m_thumbnailCallbackQ;

#ifdef SAMSUNG_LBP
    lbp_queue_t                     *m_LBPbufferQ;
    lbp_buffer_t                    m_LBPbuffer[4];
#endif
#ifdef SAMSUNG_BD
    bd_queue_t                      *m_BDbufferQ;
    UTstr                           m_BDbuffer[MAX_BD_BUFF_NUM];
    int                             m_BDbufferIndex;
#endif

    bool                            m_flagStartFaceDetection;
    bool                            m_flagLLSStart;
    bool                            m_flagLightCondition;
#ifdef SAMSUNG_CAMERA_EXTRA_INFO
    bool                            m_flagFlashCallback;
    bool                            m_flagHdrCallback;
#endif
    camera_face_t                   m_faces[NUM_OF_DETECTED_FACES];
    camera_frame_metadata_t         m_frameMetadata;
    camera_memory_t                 *m_fdCallbackHeap;

#ifdef SR_CAPTURE
    camera_face_t                   m_faces_sr[NUM_OF_DETECTED_FACES];
    camera_memory_t                 *m_srCallbackHeap;
    struct camera2_dm               m_srShotMeta;
    camera_frame_metadata_t         m_sr_frameMetadata;
    bool                            m_isCopySrMdeta;
    int                             m_sr_cropped_width;
    int                             m_sr_cropped_height;
#endif

    bool                            m_faceDetected;
    int                             m_fdThreshold;

    frame_queue_t                   *m_facedetectQ;
    sp<mainCameraThread>            m_facedetectThread;
    bool                            m_facedetectThreadFunc(void);

    ExynosCameraScalableSensor      m_scalableSensorMgr;

    /* Watch Dog Thread */
    sp<mainCameraThread>            m_monitorThread;
    bool                            m_monitorThreadFunc(void);
#ifdef MONITOR_LOG_SYNC
    static uint32_t                 cameraSyncLogId;
    int                             m_syncLogDuration;
    uint32_t                        m_getSyncLogId(void);
#endif
    bool                            m_disablePreviewCB;
    bool                            m_flagThreadStop;
    status_t                        m_checkThreadState(int *threadState, int *countRenew);
    status_t                        m_checkThreadInterval(uint32_t pipeId, uint32_t pipeInterval, int *threadState);
    unsigned int                    m_callbackState;
    unsigned int                    m_callbackStateOld;
    int                             m_callbackMonitorCount;
    bool                            m_isNeedAllocPictureBuffer;

#ifdef FPS_CHECK
    /* TODO: */
#define DEBUG_MAX_PIPE_NUM 10
    int32_t                         m_debugFpsCount[DEBUG_MAX_PIPE_NUM];
    ExynosCameraDurationTimer       m_debugFpsTimer[DEBUG_MAX_PIPE_NUM];
#endif

    ExynosCameraFrameSelector       *m_captureSelector;
    ExynosCameraFrameSelector       *m_sccCaptureSelector;

    sp<mainCameraThread>            m_jpegSaveThread[JPEG_SAVE_THREAD_MAX_COUNT];
    bool                            m_jpegSaveThreadFunc(void);

#ifdef SAMSUNG_MAGICSHOT
    int                             m_magicshotMaxCount;
#endif
    sp<mainCameraThread>            m_postPictureGscThread;
    bool                            m_postPictureGscThreadFunc(void);

#ifdef SAMSUNG_LBP
    sp<mainCameraThread>            m_LBPThread;
    bool                            m_LBPThreadFunc(void);
#endif
#ifdef SAMSUNG_LLS_DEBLUR
    bool                            m_LDCaptureThreadFunc(void);
#endif

    jpeg_callback_queue_t          *m_jpegSaveQ[JPEG_SAVE_THREAD_MAX_COUNT];

#ifdef BURST_CAPTURE
    bool                            m_isCancelBurstCapture;
    int                             m_burstCaptureCallbackCount;
    mutable Mutex                   m_burstCaptureCallbackCountLock;
    mutable Mutex                   m_burstCaptureSaveLock;
    ExynosCameraDurationTimer       m_burstSaveTimer;
    long long                       m_burstSaveTimerTime;
    int                             m_burstDuration;
    bool                            m_burstInitFirst;
    bool                            m_burstRealloc;
    char                            m_burstSavePath[CAMERA_FILE_PATH_SIZE];
    int                             m_burstShutterLocation;
#endif

#ifdef ONE_SECOND_BURST_CAPTURE
    ExynosCameraDurationTimer       TakepictureDurationTimer;
    uint64_t                        TakepictureDurationTime[ONE_SECOND_BURST_CAPTURE_CHECK_COUNT];
    bool                            m_one_second_burst_capture;
    bool                            m_one_second_burst_first_after_open;
    camera_memory_t                 *m_one_second_jpegCallbackHeap;
    camera_memory_t                 *m_one_second_postviewCallbackHeap;
#endif

    bool                            m_stopBurstShot;
    bool                            m_burst[JPEG_SAVE_THREAD_MAX_COUNT];
    bool                            m_running[JPEG_SAVE_THREAD_MAX_COUNT];

    bool                            m_isZSLCaptureOn;

#ifdef SAMSUNG_INF_BURST
    char                            m_burstTime[20];
#endif
#ifdef LLS_CAPTURE
    int                             m_needLLS_history[LLS_HISTORY_COUNT];
#endif

    frame_queue_t                   *m_ThumbnailPostCallbackQ;
    ExynosCameraBufferManager       *m_thumbnailGscBufferMgr;
    sp<mainCameraThread>            m_ThumbnailCallbackThread;
    bool                            m_ThumbnailCallbackThreadFunc(void);

    /* high resolution preview callback */
    sp<mainCameraThread>            m_highResolutionCallbackThread;
    bool                            m_highResolutionCallbackThreadFunc(void);

    frame_queue_t                   *m_highResolutionCallbackQ;
    bool                            m_highResolutionCallbackRunning;
    bool                            m_skipReprocessing;
    int                             m_skipCount;
    bool                            m_resetPreview;

    uint32_t                        m_displayPreviewToggle;

    bool                            m_hdrEnabled;
    unsigned int                    m_hdrSkipedFcount;
    bool                            m_isFirstStart;
    uint32_t                        m_dynamicSccCount;
    bool                            m_isTryStopFlash;
    uint32_t                        m_curMinFps;

    /* vision */
    status_t                        m_startVisionInternal(void);
    status_t                        m_stopVisionInternal(void);

    status_t                        m_setVisionBuffers(void);
    status_t                        m_setVisionCallbackBuffer(void);

    sp<mainCameraThread>            m_visionThread;
    bool                            m_visionThreadFunc(void);
    int                             m_visionFps;
    int                             m_visionAe;

    /* shutter callback */
    sp<mainCameraThread>            m_shutterCallbackThread;
    bool                            m_shutterCallbackThreadFunc(void);
#ifdef SAMSUNG_DOF
    void                            m_clearDOFmeta(void);
#endif
#ifdef SAMSUNG_OT
    void                            m_clearOTmeta(void);
#endif

    int                             m_previewBufferCount;
    struct ExynosConfigInfo         *m_exynosconfig;
#if 1
    uint32_t                        m_hackForAlignment;
#endif
    uint32_t                        m_recordingFrameSkipCount;

    bool                           m_mainThreadQSetup3AA_ISP();
    bool                           m_mainThreadQSetupISP();
    bool                           m_mainThreadQSetupFLITE();
    bool                           m_mainThreadQSetup3AC();
    bool                           m_mainThreadQSetupSCP();
    bool                           m_mainThreadQSetup3AA();

    status_t                       m_startCompanion(void);
    status_t                       m_stopCompanion(void);
    status_t                       m_waitCompanionThreadEnd(void);

    frame_queue_t                  *m_zoomPreviwWithCscQ;
    sp<mainCameraThread>           m_zoomPreviwWithCscThread;
    bool                           m_zoomPreviwWithCscThreadFunc(void);

#ifdef SAMSUNG_LLV
#ifdef SAMSUNG_LLV_TUNING
    int                            m_LLVpowerLevel;
    status_t                       m_checkLLVtuning(void);
#endif
    int                            m_LLVstatus;
    void                           *m_LLVpluginHandle;
#endif
#ifdef SAMSUNG_OT
    void                           *m_OTpluginHandle;
    UniPluginFocusData_t            m_OTfocusData;
    UniPluginFocusData_t            m_OTpredictedData;
    int                             m_OTstart;
    int                             m_OTstatus;
    bool                            m_OTisTouchAreaGet;
    Mutex                           m_OT_mutex;
    mutable Condition               m_OT_condition;
    bool                            m_OTisWait;
#endif
#ifdef SAMSUNG_HLV
    void                            *m_HLV;
    int                             m_HLVprocessStep;
#endif
#ifdef SAMSUNG_LBP
    int                            m_LBPindex;
    int                            m_LBPCount;
    void*                          m_LBPpluginHandle;
    bool                           m_isLBPlux;
    bool                           m_isLBPon;
#endif
#ifdef SAMSUNG_JQ
    unsigned char                  m_qtable[128];
    bool                           m_isJpegQtableOn;
    void                           *m_JQpluginHandle;
#endif
#ifdef SAMSUNG_BD
    void                           *m_BDpluginHandle;
    int                             m_BDstatus;
#endif
#ifdef SAMSUNG_OIS_VDIS
    UNI_PLUGIN_OIS_MODE             m_OISvdisMode;
#endif
#ifdef SAMSUNG_DEBLUR
    void                           *m_DeblurpluginHandle;
    void                           *m_ImgScreenerpluginHandle;
#endif
#ifdef SAMSUNG_LLS_DEBLUR
    void                           *m_LLSpluginHandle;
#endif
#ifdef SAMSUNG_DOF
    int                             m_lensmoveCount;
    int                             m_AFstatus;
    bool                            m_currentSetStart;
    bool                            m_flagMetaDataSet;
#endif
#ifdef SAMSUNG_SENSOR_LISTENER
    sp<mainCameraThread>            m_sensorListenerThread;
    bool                            m_sensorListenerThreadFunc(void);
#ifdef SAMSUNG_HRM
    void*                           m_uv_rayHandle;
    SensorListenerEvent_t           m_uv_rayListenerData;
#endif
#ifdef SAMSUNG_LIGHT_IR
    void*                           m_light_irHandle;
    SensorListenerEvent_t           m_light_irListenerData;
#endif
#ifdef SAMSUNG_GYRO
    void*                           m_gyroHandle;
    SensorListenerEvent_t           m_gyroListenerData;
#endif
#endif

#ifdef FIRST_PREVIEW_TIME_CHECK
    ExynosCameraDurationTimer       m_firstPreviewTimer;
    bool                            m_flagFirstPreviewTimerOn;
#endif
#ifdef SAMSUNG_QUICKSHOT
    bool                            m_quickShotStart;
#endif
#ifdef CAMERA_FAST_ENTRANCE_V1
    bool                            m_isFirstParametersSet;
    bool                            m_fastEntrance;
    int                             m_fastenAeThreadResult;
    sp<mainCameraThread>            m_fastenAeThread;
    bool                            m_fastenAeThreadFunc(void);
    status_t                        m_waitFastenAeThreadEnd(void);
#endif
#ifdef USE_CAMERA_PREVIEW_FRAME_SCHEDULER
    sp<SecCameraPreviewFrameScheduler> m_previewFrameScheduler;
#endif

    mutable Mutex                   m_metaCopyLock;
    struct camera2_shot_ext         *m_tempshot;
    struct camera2_shot_ext         *m_fdmeta_shot;
    struct camera2_shot_ext         *m_meta_shot;

#ifdef SAMSUNG_DEBLUR
    bool                            m_useDeblurCaptureOn;
    int                             m_deblurCaptureCount;
    ExynosCameraFrame               *m_deblur_firstFrame;
    ExynosCameraFrame               *m_deblur_secondFrame;
    sp<mainCameraThread>            m_detectDeblurCaptureThread;
    sp<mainCameraThread>            m_deblurCaptureThread;
    deblur_capture_queue_t          *m_detectDeblurCaptureQ;
    frame_queue_t                   *m_deblurCaptureQ;
#endif

#ifdef SAMSUNG_LLS_DEBLUR
    sp<mainCameraThread>            m_LDCaptureThread;
    frame_queue_t                   *m_LDCaptureQ;
    int                             m_LDCaptureCount;
    int                             m_LDBufIndex[MAX_LD_CAPTURE_COUNT];
#endif
    int                             mIonClient;
};

}; /* namespace android */
#endif

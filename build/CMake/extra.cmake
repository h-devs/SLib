set (SLIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")

include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories (
 "${SLIB_PATH}/include"
)
link_directories(
 "${SLIB_LIB_PATH}"
)

set (EXTRA_SOURCE_FILES

 "${SLIB_PATH}/extra/captcha/captcha.cpp"

 "${SLIB_PATH}/extra/chat/chat.cpp"
 "${SLIB_PATH}/extra/chat/chat_client.cpp"
 "${SLIB_PATH}/extra/chat/chat_resource.cpp"
 "${SLIB_PATH}/extra/chat/chat_sqlite.cpp"
 "${SLIB_PATH}/extra/chat/chat_view.cpp"

 "${SLIB_PATH}/extra/keygen/keygen.cpp"

 "${SLIB_PATH}/extra/push_notification/fcm.cpp"
 "${SLIB_PATH}/extra/push_notification/fcm_service.cpp"
 "${SLIB_PATH}/extra/push_notification/xgpush.cpp"
 "${SLIB_PATH}/extra/push_notification/xgpush_service.cpp"

 "${SLIB_PATH}/extra/qrcode/zxing.cpp"
 "${SLIB_PATH}/extra/qrcode/zxing_scanner.cpp"

 "${SLIB_PATH}/extra/social/alipay.cpp"
 "${SLIB_PATH}/extra/social/alipay_openssl.cpp"
 "${SLIB_PATH}/extra/social/alipay_sdk.cpp"
 "${SLIB_PATH}/extra/social/ebay.cpp"
 "${SLIB_PATH}/extra/social/etsy.cpp"
 "${SLIB_PATH}/extra/social/etsy_ui.cpp"
 "${SLIB_PATH}/extra/social/facebook.cpp"
 "${SLIB_PATH}/extra/social/facebook_sdk.cpp"
 "${SLIB_PATH}/extra/social/facebook_ui.cpp"
 "${SLIB_PATH}/extra/social/instagram.cpp"
 "${SLIB_PATH}/extra/social/linkedin.cpp"
 "${SLIB_PATH}/extra/social/linkedin_ui.cpp"
 "${SLIB_PATH}/extra/social/paypal.cpp"
 "${SLIB_PATH}/extra/social/paypal_ui.cpp"
 "${SLIB_PATH}/extra/social/pinterest.cpp"
 "${SLIB_PATH}/extra/social/pinterest_ui.cpp"
 "${SLIB_PATH}/extra/social/twitter.cpp"
 "${SLIB_PATH}/extra/social/wechat.cpp"
 "${SLIB_PATH}/extra/social/wechat_sdk.cpp"

)

if (ANDROID)
 set (EXTRA_PLATFORM_FILES

  "${SLIB_PATH}/extra/push_notification/fcm_android.cpp"
  "${SLIB_PATH}/extra/push_notification/xgpush_android.cpp"

  "${SLIB_PATH}/extra/social/alipay_android.cpp"
  "${SLIB_PATH}/extra/social/facebook_android.cpp"
  "${SLIB_PATH}/extra/social/instagram_android.cpp"
  "${SLIB_PATH}/extra/social/wechat_android.cpp"

 )
else()
 set (EXTRA_PLATFORM_FILES
 )
endif()

add_library (
 slib-extra STATIC
 ${EXTRA_SOURCE_FILES}
 ${EXTRA_PLATFORM_FILES}
)
set_target_properties (
 slib-extra
 PROPERTIES
 ARCHIVE_OUTPUT_DIRECTORY "${SLIB_LIB_PATH}"
)

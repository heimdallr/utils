AddTarget(
	NAME FindInvalidImages
	TYPE app_console
	PROJECT_GROUP Tool
	SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/FindInvalidImages"
	QT_USE
		Core Gui Widgets
	QT_PLUGINS
		QJpeg
	MODULES
		qt
	COMPILER_OPTIONS
		[ MSVC /WX /W4 ]
)

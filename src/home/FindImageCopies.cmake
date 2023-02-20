AddTarget(
	NAME FindImageCopies
	TYPE app_console
	PROJECT_GROUP Tool
	SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/FindImageCopies"
	QT_USE
		Core
	QT_PLUGINS
		QJpeg
	MODULES
		qt
	COMPILER_OPTIONS
		[ MSVC /WX /W4 ]
)

AddTarget(
	NAME FindCopies
	TYPE app_console
	PROJECT_GROUP Tool
	SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/FindCopies"
	QT_USE
		Core
	QT_PLUGINS
		QJpeg
	MODULES
		qt
	COMPILER_OPTIONS
		[ MSVC /WX /W4 ]
)

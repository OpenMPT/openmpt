namespace pfc {
	namespace io {
		namespace path {
#ifdef _WINDOWS
			typedef string::comparatorCaseInsensitive comparator;
#else
            typedef string::comparatorCaseSensitive comparator; // wild assumption
#endif
            

			string getFileName(string path);
			string getFileNameWithoutExtension(string path);
			string getFileExtension(string path);
			string getParent(string filePath);
			string getDirectory(string filePath);//same as getParent()
			string combine(string basePath,string fileName);
			char getDefaultSeparator();
			string getSeparators();
			bool isSeparator(char c);
			string getIllegalNameChars(bool allowWC = false);
			string replaceIllegalNameChars(string fn, bool allowWC = false);
			string replaceIllegalPathChars(string fn);
			bool isInsideDirectory(pfc::string directory, pfc::string inside);
			bool isDirectoryRoot(string path);
			string validateFileName(string name, bool allowWC = false);//removes various illegal things from the name, exact effect depends on the OS, includes removal of the invalid characters

			template<typename t1, typename t2> inline bool equals(const t1 & v1, const t2 & v2) {return comparator::compare(v1,v2) == 0;}

            template<typename t1, typename t2> inline int compare( t1 const & p1, t2 const & p2 ) {return comparator::compare(p1, p2); }
		}
	}
}

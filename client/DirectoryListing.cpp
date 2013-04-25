/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DirectoryListing.h"

#include "QueueManager.h"
#include "SearchManager.h"

#include "StringTokenizer.h"
#include "SimpleXML.h"
#include "FilteredFile.h"
#include "BZUtils.h"
#include "CryptoManager.h"
#include "ResourceManager.h"
#include "SimpleXMLReader.h"
#include "User.h"
#include "ShareManager.h"


namespace dcpp
{

DirectoryListing::DirectoryListing(const HintedUser& aUser) :
	hintedUser(aUser), abort(false), root(new Directory(NULL, Util::emptyString, false, false))
{
}

DirectoryListing::~DirectoryListing()
{
	delete root;
}

UserPtr DirectoryListing::getUserFromFilename(const string& fileName)
{
	// General file list name format: [username].[CID].[xml|xml.bz2]
	
	string name = Util::getFileName(fileName);
	
	// Strip off any extensions
	if (stricmp(name.c_str() + name.length() - 4, ".bz2") == 0)
	{
		name.erase(name.length() - 4);
	}
	
	if (stricmp(name.c_str() + name.length() - 4, ".xml") == 0)
	{
		name.erase(name.length() - 4);
	}
	
	// Find CID
	string::size_type i = name.rfind('.');
	if (i == string::npos)
	{
		return UserPtr();
	}
	
	size_t n = name.length() - (i + 1);
	// CID's always 39 chars long...
	if (n != 39)
		return UserPtr();
		
	CID cid(name.substr(i + 1));
	if (cid.isZero())
		return UserPtr();
		
	return ClientManager::getInstance()->getUser(cid);
}

void DirectoryListing::loadFile(const string& name, bool p_own_list)
{

	// For now, we detect type by ending...
	string ext = Util::getFileExt(name);
	
	dcpp::File ff(name, dcpp::File::READ, dcpp::File::OPEN);
	if (stricmp(ext, ".bz2") == 0)
	{
		FilteredInputStream<UnBZFilter, false> f(&ff);
		loadXML(f, false, p_own_list);
	}
	else if (stricmp(ext, ".xml") == 0)
	{
		loadXML(ff, false, p_own_list);
	}
}

class ListLoader : public SimpleXMLReader::CallBack
{
	public:
		ListLoader(DirectoryListing* aList, DirectoryListing::Directory* root, bool aUpdating, const UserPtr& aUser, bool p_own_list)
			: list(aList), cur(root), base("/"), inListing(false), updating(aUpdating), user(aUser), m_own_list(p_own_list)
		{
		}
		
		~ListLoader() { }
		
		void startTag(const string& name, StringPairList& attribs, bool simple);
		void endTag(const string& name, const string& data);
		
		const string& getBase() const
		{
			return base;
		}
	private:
		DirectoryListing* list;
		DirectoryListing::Directory* cur;
		UserPtr user;
		
		StringMap params;
		string base;
		bool inListing;
		bool updating;
		bool m_own_list;
};

string DirectoryListing::updateXML(const string& xml, bool p_own_list)
{
	MemoryInputStream mis(xml);
	return loadXML(mis, true, p_own_list);
}

string DirectoryListing::loadXML(InputStream& is, bool updating, bool p_own_list)
{
	ListLoader ll(this, getRoot(), updating, getUser(), p_own_list);
	
	dcpp::SimpleXMLReader(&ll).parse(is);
	
	return ll.getBase();
}

static const string sFileListing = "FileListing";
static const string sBase = "Base";
static const string sGenerator = "Generator";
static const string sDirectory = "Directory";
static const string sIncomplete = "Incomplete";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sTTH = "TTH";
static const string sHIT = "HIT";
static const string sTS = "TS";
static const string sBR = "BR";
static const string sWH = "WH";
static const string sMVideo = "MV";
static const string sMAudio = "MA";

void ListLoader::startTag(const string& name, StringPairList& attribs, bool simple)
{
	if (list->getAbort())
	{
		throw AbortException();
	}
	
	if (inListing)
	{
		if (name == sFile)
		{
			const string& n = getAttrib(attribs, sName, 0);
			if (n.empty())
				return;
				
			const string& s = getAttrib(attribs, sSize, 1);
			if (s.empty())
				return;
			auto size = Util::toInt64(s);
			
			const string& h = getAttrib(attribs, sTTH, 2);
			if (h.empty())
				return;
			TTHValue tth(h); /// @todo verify validity?
			
			if (updating)
			{
				// just update the current file if it is already there.
				for (auto i = cur->files.cbegin(), iend = cur->files.cend(); i != iend; ++i)
				{
					auto& file = **i;
					/// @todo comparisons should be case-insensitive but it takes too long - add a cache
					if (file.getTTH() == tth || file.getName() == n)
					{
						file.setName(n);
						file.setSize(size);
						file.setTTH(tth);
						return;
					}
				}
			}
			string l_hit;
			string l_br;
			CFlyMediaInfo l_mediaXY;
			const int l_i_hit = l_hit.empty() ? 0 : atoi(l_hit.c_str());
			const string& l_ts = getAttrib(attribs, sTS, 3);
			if (l_ts.size()) // Extended tags - exists only FlylinkDC++ or StrongDC++ sqlite or clones
			{
				l_hit = getAttrib(attribs, sHIT, 3);
				l_br = getAttrib(attribs, sBR, 4);
				l_mediaXY.init(getAttrib(attribs, sWH, 3), atoi(l_br.c_str()));
				l_mediaXY.m_audio = getAttrib(attribs, sMAudio, 3);
				l_mediaXY.m_video = getAttrib(attribs, sMVideo, 3);
			}
			DirectoryListing::File* f = new DirectoryListing::File(cur, n, Util::toInt64(s), h, l_i_hit, atoi(l_ts.c_str()), l_mediaXY);
			cur->files.push_back(f);
			if (!m_own_list && ShareManager::getInstance()->isTTHShared(f->getTTH()))
				f->setFlag(DirectoryListing::FLAG_SHARED);
			else
			{
				f->setFlag(DirectoryListing::FLAG_NOT_SHARED);
				if (!m_own_list && CFlylinkDBManager::getInstance()->is_download_tth(f->getTTH()))
					f->setFlag(DirectoryListing::FLAG_DOWNLOAD);
				else if (!m_own_list && CFlylinkDBManager::getInstance()->is_old_tth(f->getTTH()))
					f->setFlag(DirectoryListing::FLAG_OLD_TTH);
					
			}
		}
		else if (name == sDirectory)
		{
			const string& n = getAttrib(attribs, sName, 0);
			if (n.empty())
			{
				throw SimpleXMLException("Directory missing name attribute");
			}
			bool incomp = getAttrib(attribs, sIncomplete, 1) == "1";
			DirectoryListing::Directory* d = NULL;
			if (updating)
			{
				for (DirectoryListing::Directory::Iter i  = cur->directories.begin(); i != cur->directories.end(); ++i)
				{
					/// @todo comparisons should be case-insensitive but it takes too long - add a cache
					if ((*i)->getName() == n)
					{
						d = *i;
						if (!d->getComplete())
							d->setComplete(!incomp);
						break;
					}
				}
			}
			if (d == NULL)
			{
				d = new DirectoryListing::Directory(cur, n, false, !incomp);
				cur->directories.push_back(d);
			}
			cur = d;
			
			if (simple)
			{
				// To handle <Directory Name="..." />
				endTag(name, Util::emptyString);
			}
		}
	}
	else if (name == sFileListing)
	{
		const string& b = getAttrib(attribs, sBase, 2);
		if (b.size() >= 1 && b[0] == '/' && b[b.size() - 1] == '/')
		{
			base = b;
		}
		if (base.size() > 1) // [+]PPA fix for [4](("Version", "1"),("CID", "EDI7OWB6TZWH6X6L2D3INC6ORQSG6RQDJ6AJ5QY"),("Base", "/"),("Generator", "DC++ 0.785"))
		{
			StringList sl = StringTokenizer<string>(base.substr(1), '/').getTokens();
			for (StringIter i = sl.begin(); i != sl.end(); ++i)
			{
				DirectoryListing::Directory* d = NULL;
				for (DirectoryListing::Directory::Iter j = cur->directories.begin(); j != cur->directories.end(); ++j)
				{
					if ((*j)->getName() == *i)
					{
						d = *j;
						break;
					}
				}
				if (d == NULL)
				{
					d = new DirectoryListing::Directory(cur, *i, false, false);
					cur->directories.push_back(d);
				}
				cur = d;
			}
		}
		cur->setComplete(true);
		
		string generator = getAttrib(attribs, sGenerator, 2);
		ClientManager::getInstance()->setGenerator(user, generator);
		
		inListing = true;
		
		if (simple)
		{
			// To handle <Directory Name="..." />
			endTag(name, Util::emptyString);
		}
	}
}

void ListLoader::endTag(const string& name, const string&)
{
	if (inListing)
	{
		if (name == sDirectory)
		{
			cur = cur->getParent();
		}
		else if (name == sFileListing)
		{
			// cur should be root now...
			inListing = false;
		}
	}
}

string DirectoryListing::getPath(const Directory* d) const
{
	if (d == root)
		return "";
		
	string dir;
	dir.reserve(128);
	dir.append(d->getName());
	dir.append(1, '\\');
	
	Directory* cur = d->getParent();
	while (cur != root)
	{
		dir.insert(0, cur->getName() + '\\');
		cur = cur->getParent();
	}
	return dir;
}

void DirectoryListing::download(Directory* aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio)
{

	string target = (aDir == getRoot()) ? aTarget : aTarget + aDir->getName() + PATH_SEPARATOR;
	
	if (!aDir->getComplete())
	{
		// folder is not completed (partial list?), so we need to download it first
		QueueManager::getInstance()->addDirectory("", hintedUser, target, prio);
	}
	else
	{
		// First, recurse over the directories
		Directory::List& lst = aDir->directories;
		sort(lst.begin(), lst.end(), Directory::DirSort());
		for (Directory::Iter j = lst.begin(); j != lst.end(); ++j)
		{
			download(*j, target, highPrio, prio);
		}
		// Then add the files
		File::List& l = aDir->files;
		sort(l.begin(), l.end(), File::FileSort());
		for (File::Iter i = aDir->files.begin(); i != aDir->files.end(); ++i)
		{
			File* file = *i;
			try
			{
				download(file, target + file->getName(), false, highPrio, prio);
			}
			catch (const QueueException&)
			{
				// Catch it here to allow parts of directories to be added...
			}
			catch (const FileException&)
			{
				//..
			}
		}
	}
}

void DirectoryListing::download(const string& aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio)
{
	if (aDir.size() <= 2)
	{
		LogManager::getInstance()->message("[error] DirectoryListing::download aDir.size() <= 2 aDir=" + aDir + " aTarget = " + aTarget);
		return;
	}
	dcassert(aDir.size() > 2); // todo aDir IS NULL
	dcassert(aDir[aDir.size() - 1] == '\\'); // This should not be PATH_SEPARATOR
	Directory* d = find(aDir, getRoot());
	if (d != NULL)
		download(d, aTarget, highPrio, prio);
}

void DirectoryListing::download(File* aFile, const string& aTarget, bool view, bool highPrio, QueueItem::Priority prio)
{
	Flags::MaskType flags = (Flags::MaskType)(view ? (QueueItem::FLAG_TEXT | QueueItem::FLAG_CLIENT_VIEW) : 0);
	
	QueueManager::getInstance()->add(aTarget, aFile->getSize(), aFile->getTTH(), getHintedUser(), flags);
	
	if (highPrio || (prio != QueueItem::DEFAULT))
		QueueManager::getInstance()->setPriority(aTarget, highPrio ? QueueItem::HIGHEST : prio);
}

DirectoryListing::Directory* DirectoryListing::find(const string& aName, Directory* current)
{
	string::size_type end = aName.find('\\');
	dcassert(end != string::npos);
	string name = aName.substr(0, end);
	
	Directory::Iter i = std::find(current->directories.begin(), current->directories.end(), name);
	if (i != current->directories.end())
	{
		if (end == (aName.size() - 1))
			return *i;
		else
			return find(aName.substr(end + 1), *i);
	}
	return NULL;
}
void DirectoryListing::log_matched_files(const HintedUser& p_user, int p_count) //[+]PPA
{
	const size_t l_BUF_SIZE = STRING(MATCHED_FILES).size() + 16;
	string l_tmp;
	l_tmp.resize(l_BUF_SIZE);
	snprintf(&l_tmp[0], l_tmp.size(), CSTRING(MATCHED_FILES), p_count);
	LogManager::getInstance()->message(Util::toString(ClientManager::getInstance()->getNicks(p_user)) + string(": ") + l_tmp.c_str());
}

struct HashContained
{
		HashContained(const DirectoryListing::Directory::TTHSet& l) : tl(l) { }
		const DirectoryListing::Directory::TTHSet& tl;
		bool operator()(const DirectoryListing::File::Ptr i) const
		{
			return tl.count((i->getTTH())) && (DeleteFunction()(i), true);
		}
	private:
		HashContained& operator=(HashContained&);
};

struct DirectoryEmpty
{
	bool operator()(const DirectoryListing::Directory::Ptr i) const
	{
		bool r = i->getFileCount() + i->directories.size() == 0;
		if (r) DeleteFunction()(i);
		return r;
	}
};

DirectoryListing::Directory::~Directory()
{
	for_each(directories.begin(), directories.end(), DeleteFunction());
	for_each(files.begin(), files.end(), DeleteFunction());
}

void DirectoryListing::Directory::filterList(DirectoryListing& dirList)
{
	DirectoryListing::Directory* d = dirList.getRoot();
	
	TTHSet l;
	d->getHashList(l);
	filterList(l);
}

void DirectoryListing::Directory::filterList(DirectoryListing::Directory::TTHSet& l)
{
	for (Iter i = directories.begin(); i != directories.end(); ++i)(*i)->filterList(l);
	directories.erase(std::remove_if(directories.begin(), directories.end(), DirectoryEmpty()), directories.end());
	files.erase(std::remove_if(files.begin(), files.end(), HashContained(l)), files.end());
}

void DirectoryListing::Directory::getHashList(DirectoryListing::Directory::TTHSet& l)
{
	for (Iter i = directories.begin(); i != directories.end(); ++i)(*i)->getHashList(l);
	for (DirectoryListing::File::Iter i = files.begin(); i != files.end(); ++i) l.insert((*i)->getTTH());
}

uint32_t DirectoryListing::Directory::getTotalTS() const
{
	uint32_t x = getTS();
	for (Iter i = directories.begin(); i != directories.end(); ++i)
	{
		x = std::max((*i)->getTS(), x);
	}
	return x;
}

uint16_t DirectoryListing::Directory::getTotalBitrate() const
{
	uint16_t x = getBitrate();
	for (Iter i = directories.begin(); i != directories.end(); ++i)
	{
		x = std::max((*i)->getBitrate(), x);
	}
	return x;
}

int64_t DirectoryListing::Directory::getTotalHit() const
{
	int64_t x = getHit();
	for (Iter i = directories.begin(); i != directories.end(); ++i)
	{
		x += (*i)->getTotalHit();
	}
	return x;
}
int64_t DirectoryListing::Directory::getTotalSize(bool adl) const
{
	int64_t x = getSize();
	for (Iter i = directories.begin(); i != directories.end(); ++i)
	{
		if (!(adl && (*i)->getAdls()))
			x += (*i)->getTotalSize(adls);
	}
	return x;
}

size_t DirectoryListing::Directory::getTotalFileCount(bool adl) const
{
	size_t x = getFileCount();
	for (Iter i = directories.begin(); i != directories.end(); ++i)
	{
		if (!(adl && (*i)->getAdls()))
			x += (*i)->getTotalFileCount(adls);
	}
	return x;
}

// !fulDC! !SMT!-UI
void DirectoryListing::Directory::checkDupes(const DirectoryListing* lst)
{
	Flags::MaskType result = 0;
	for (Directory::Iter i = directories.begin(); i != directories.end(); ++i)
	{
		(*i)->checkDupes(lst);
		result |= (*i)->getFlags() & (
		              FLAG_OLD_TTH | FLAG_DOWNLOAD | FLAG_SHARED | FLAG_NOT_SHARED);
	}
	for (File::Iter i = files.begin(); i != files.end(); ++i)
	{
		//don't count 0 byte files since it'll give lots of partial dupes
		//of no interest
		if ((*i)->getSize() > 0)
		{
			result |= (*i)->getFlags() & (
			              FLAG_OLD_TTH | FLAG_DOWNLOAD |  FLAG_SHARED | FLAG_NOT_SHARED);
		}
	}
	setFlags(result);
}

// !SMT!-UI
void DirectoryListing::checkDupes()
{
	root->checkDupes(this);
}
} // namespace dcpp

/**
 * @file
 * $Id: DirectoryListing.cpp 568 2011-07-24 18:28:43Z bigmuscle $
 */

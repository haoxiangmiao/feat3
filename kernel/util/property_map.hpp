#pragma once
#ifndef KERNEL_UTIL_PROPERTY_MAP_HPP
#define KERNEL_UTIL_PROPERTY_MAP_HPP 1

// includes, FEAST
#include <kernel/util/file_error.hpp>

// includes, system
#include <iostream>
#include <map>

namespace FEAST
{
  /**
   * \brief A class organising a tree of key-value pairs
   *
   * \see \ref ini_format
   *
   * \author Constantin Christof
   * \author Peter Zajac
   */
  class PropertyMap
  {
  public:
    /// entry-map type
    typedef std::map<String, String, String::NoCaseLess> EntryMap;
    /// section-map type
    typedef std::map<String, PropertyMap*, String::NoCaseLess> SectionMap;

    /// entry iterator type
    typedef EntryMap::iterator EntryIterator;
    /// const entry iterator type
    typedef EntryMap::const_iterator ConstEntryIterator;
    /// section iterator type
    typedef SectionMap::iterator SectionIterator;
    /// const section iterator type
    typedef SectionMap::const_iterator ConstSectionIterator;

  protected:
    /// a map storing the key-value-pairs
    EntryMap _values;

    /// a map storing the sub-sections
    SectionMap _sections;

  public:
    /// Default Constructor
    PropertyMap();

    /// Virtual Destructor
    virtual ~PropertyMap();

    /**
     * \brief Adds a new key-value pair to this map.
     *
     * This function adds a new key-value pair to this section or, if desired, overwrites an existing one.
     *
     * \param[in] key
     * A String containing the key of the entry.
     *
     * \param[in] value
     * A String containing the value of the entry.
     *
     * \param[in] replace
     * Specifies the behaviour when an entry with the same key already exists in the section:
     *  - If \p replace = \c true, then the old value of the entry is replaced by the \p value parameter passed
     *    to this function.
     *  - If \p replace = \c false, then the old value of the entry is kept and this function returns \c false.
     *
     * \returns
     * \c true, if the key-value-pair has been stored or \c false, if an entry with the key already exists and
     * \p replace was set to \c false.
     */
    bool add_entry(String key, String value, bool replace = true);

    /**
     * \brief Adds a new sub-section to this map.
     *
     * This function adds a new sub-section and returns a pointer to it. If a sub-section with the corresponding
     * name already exists, a pointer to that section is returned instead of allocating a new one.
     *
     * \param[in] name
     * The name of the section to be added.
     *
     * \return
     * A pointer to a the \c PropertyMap associated to the name.
     */
    PropertyMap* add_section(String name);

    /**
     * \brief Erases a key-value pair.
     *
     * \param[in] key
     * The key of the entry to be erased.
     *
     * \returns
     * \c true if the entry was erased or \c false if no entry with that key was found.
     */
    bool erase_entry(String key);

    /**
     * \brief Erases a sub-section
     *
     * \param[in] name
     * The name of the sub-section to be erased.
     *
     * \returns
     * \c true if the section was erased or \c false if no section with that name was found.
     */
    bool erase_section(String name);

    /**
     * \brief Queries a value.
     *
     * \param[in] key_path
     * A path to the key whose value is to be returned.
     *
     * \returns
     * A <c>pair<String,bool></c>, where the second component marks, whether an entry with the key path
     * has been found or not. If the <c>bool</c>-component is \c true, then the <c>String</c>-component
     * contains the value associated with the key, otherwise the <c>String</c>-component is empty.
     */
    std::pair<String, bool> query(String key_path) const;

    /**
     * \brief Queries a value.
     *
     * \param[in] key_path
     * A path to the key whose value is to be returned.
     *
     * \param[in] default_value
     * A string that is to be returned in the case that the key was not found.
     *
     * \returns
     * A String containing the value corresponding to the key-path, or \p default_value if no such key was found.
     */
    String query(String key_path, String default_value) const;

    /**
     * \brief Retrieves a value for a given key.
     *
     * This function returns the value string of a key-value pair.
     *
     * \param[in] key
     * The key of the entry whose value is to be returned.
     *
     * \returns
     * A <c>pair<String, bool></c>, where the second component marks, whether an entry with the \c key has been
     * found or not. If the <c>bool</c>-component is \c true, then the <c>String</c>-component contains the value
     * associated with the key, otherwise the <c>String</c>-component is empty.
     */
    std::pair<String, bool> get_entry(String key) const;

    /**
     * \brief Returns a sub-section.
     *
     * \param[in] name
     * The name of the sub-section which is to be returned.
     *
     * \returns
     * A pointer to the PropertyMap associated with \p name or \c nullptr if no section with that name exists.
     */
    PropertyMap* get_section(String name);

    /** \copydoc get_section() */
    const PropertyMap* get_section(String name) const;

    /**
     * \brief Returns a reference to the entry map.
     * \returns
     * A (const) reference to the entry map.
     */
    EntryMap& get_entry_map()
    {
      CONTEXT("PropertyMap::get_entry_map()");
      return _values;
    }

    /** \copydoc get_entry_map() */
    const EntryMap& get_entry_map() const
    {
      CONTEXT("PropertyMap::get_entry_map() [const]");
      return _values;
    }

    /// Returns the first entry iterator.
    EntryIterator begin_entry()
    {
      CONTEXT("PropertyMap::begin_entry()");
      return _values.begin();
    }

    /** \copydoc begin_entry() */
    ConstEntryIterator begin_entry() const
    {
      CONTEXT("PropertyMap::begin_entry() [const]");
      return _values.begin();
    }

    /// Returns the last entry iterator.
    EntryIterator end_entry()
    {
      CONTEXT("PropertyMap::end_entry()");
      return _values.end();
    }

    /** \copydoc end_entry() */
    ConstEntryIterator end_entry() const
    {
      CONTEXT("PropertyMap::end_entry() const");
      return _values.end();
    }

    /// Returns the first section iterator.
    SectionIterator begin_section()
    {
      CONTEXT("PropertyMap::begin_section()");
      return _sections.begin();
    }

    /** \copydoc begin_section() */
    ConstSectionIterator begin_section() const
    {
      CONTEXT("PropertyMap::begin_section() [const]");
      return _sections.begin();
    }

    /// Returns the last section iterator
    SectionIterator end_section()
    {
      CONTEXT("PropertyMap::end_section()");
      return _sections.end();
    }

    /** \copydoc end_section() */
    ConstSectionIterator end_section() const
    {
      CONTEXT("PropertyMap::end_section() [const]");
      return _sections.end();
    }

    /**
     * \brief Parses a file in INI-format.
     *
     * \param[in] filename
     * The name of the file to be parsed.
     *
     * \param[in] replace
     * Specifies the behaviour when an entry with the same key already exists:
     *  - If \p replace = \c true, then the old value of the entry is replaced by the new \p value
     *  - If \p replace = \c false, then the old value of the entry is kept.
     *
     * \see PropertyMap::parse(std::istream&)
     */
    void parse(String filename, bool replace = true);

    /**
     * \brief Parses an input stream.
     *
     * \param[in] ifs
     * A reference to an input stream to be parsed.
     *
     * \param[in] replace
     * Specifies the behaviour when an entry with the same key already exists:
     *  - If \p replace = \c true, then the old value of the entry is replaced by the new \p value
     *  - If \p replace = \c false, then the old value of the entry is kept.
     */
    void parse(std::istream& ifs, bool replace = true);

    /**
     * \brief Merges another PropertyMap into \c this.
     *
     * \param[in] section
     * The section to be merged into \c this.
     *
     * \param[in] replace
     * Specifies the behaviour when conflicting key-value pairs are encountered. See add_entry() for more details.
     */
    void merge(const PropertyMap& section, bool replace = true);

    /**
     * \brief Dumps the section tree into a file.
     *
     * This function writes the whole section tree into a file, which can be parsed by the parse() function.
     *
     * \param[in] filename
     * The name of the file into which to dump to.
     *
     * \see PropertyMap::dump(std::ostream&, String::size_type) const
     */
    void dump(String filename) const;

    /**
     * \brief Dumps the section tree into an output stream.
     *
     * This function writes the whole section tree into an output stream, which can be parsed.
     *
     * \param[in,out] os
     * A refernce to an output stream to which to write to. It is silently assumed that the output stream is
     * open.
     *
     * \param[in] indent
     * Specifies the number of indenting whitespaces to be inserted at the beginning of each line.
     *
     * \note
     * The \p indent parameter is used internally for output formatting and does not need to be specified by the
     * caller.
     */
    void dump(std::ostream& os, String::size_type indent = 0) const;
  }; // class PropertyMap
} // namespace FEAST

#endif // KERNEL_UTIL_PROPERTY_MAP_HPP
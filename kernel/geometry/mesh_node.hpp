#pragma once
#ifndef KERNEL_GEOMETRY_MESH_NODE_HPP
#define KERNEL_GEOMETRY_MESH_NODE_HPP 1

// includes, FEAST
#include <kernel/geometry/cell_sub_set_node.hpp>
#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/geometry/conformal_sub_mesh.hpp>
#include <kernel/geometry/mesh_streamer_factory.hpp>
#include <kernel/util/mesh_streamer.hpp>

// includes, STL
#include <map>

namespace FEAST
{
  namespace Geometry
  {
    /// \cond internal
    // dummy chart class; will be replaced by 'real' charts later...
    class DummyChart
    {
    public:
      template<typename A_, typename B_>
      void adapt(A_&, const B_&) const
      {
      }
    };
    /// \endcond

    /**
     * \brief Standard MeshNode Policy for ConformalMesh
     *
     * \tparam Shape_
     * The shape tag class for the mesh.
     */
    template<typename Shape_>
    struct StandardConformalMeshNodePolicy
    {
      /// root mesh type
      typedef ConformalMesh<Shape_> RootMeshType;
      /// chart for the root mesh
      typedef DummyChart RootMeshChartType;

      /// sub-mesh type
      typedef ConformalSubMesh<Shape_> SubMeshType;
      /// chart for the sub-mesh
      typedef DummyChart SubMeshChartType;

      /// cell subset type
      typedef CellSubSet<Shape_> CellSubSetType;
    };

    /// \cond internal
    // helper policy template for RootMeshNode class template
    template<typename MeshNodePolicy_>
    struct RootMeshNodePolicy
    {
      typedef typename MeshNodePolicy_::RootMeshType MeshType;
      typedef typename MeshNodePolicy_::RootMeshChartType ChartType;
    };

    // helper policy template for SubMeshNode class template
    template<typename MeshNodePolicy_>
    struct SubMeshNodePolicy
    {
      typedef typename MeshNodePolicy_::SubMeshType MeshType;
      typedef typename MeshNodePolicy_::SubMeshChartType ChartType;
    };

    // forward declarations
    template<typename Policy_>
    class SubMeshNode DOXY({});
    /// \endcond

    /**
     * \brief Mesh Node base class
     *
     * A MeshNode is a container for bundling a mesh with SubMeshes and CellSubSets referring to it.
     *
     * \author Peter Zajac
     */
    template<
      typename Policy_,
      typename MeshNodePolicy_>
    class MeshNode
      : public CellSubSetParent<Policy_>
    {
    public:
      /// base class typedef
      typedef CellSubSetParent<Policy_> BaseClass;

      /// submesh type
      typedef typename Policy_::SubMeshType SubMeshType;
      /// submesh node type
      typedef SubMeshNode<Policy_> SubMeshNodeType;

      /// mesh type of this node
      typedef typename MeshNodePolicy_::MeshType MeshType;
      /// mesh chart type of this node
      typedef typename MeshNodePolicy_::ChartType MeshChartType;

    protected:
      /**
       * \brief SubMeshNode bin class
       */
      class SubMeshNodeBin
      {
      public:
        SubMeshNodeType* node;
        const MeshChartType* chart;

      public:
        explicit SubMeshNodeBin(
          SubMeshNodeType* node_,
          const MeshChartType* chart_)
           :
          node(node_),
          chart(chart_)
        {
        }
      };

      /// submesh node bin container type
      typedef std::map<Index,SubMeshNodeBin> SubMeshNodeContainer;
      /// submesh node iterator type
      typedef typename SubMeshNodeContainer::iterator SubMeshNodeIterator;
      /// submesh node const-iterator type
      typedef typename SubMeshNodeContainer::const_iterator SubMeshNodeConstIterator;
      /// submesh node reverse-iterator type
      typedef typename SubMeshNodeContainer::reverse_iterator SubMeshNodeReverseIterator;

    protected:
      /// a pointer to the mesh of this node
      MeshType* _mesh;
      /// child submesh nodes
      SubMeshNodeContainer _submesh_nodes;

    protected:
      /**
       * \brief Constructor.
       *
       * \param[in] mesh
       * A pointer to the mesh for this node.
       */
      explicit MeshNode(MeshType* mesh) :
        BaseClass(),
        _mesh(mesh)
      {
        CONTEXT(name() + "::MeshNode()");
      }

    public:
      /// virtual destructor
      virtual ~MeshNode()
      {
        CONTEXT(name() + "::~MeshNode()");

        // loop over all submesh nodes in reverse order and delete them
        SubMeshNodeReverseIterator it(_submesh_nodes.rbegin());
        SubMeshNodeReverseIterator jt(_submesh_nodes.rend());
        for(; it != jt; ++it)
        {
          if(it->second.node != nullptr)
          {
            delete it->second.node;
          }
        }

        // delete mesh
        if(_mesh != nullptr)
        {
          delete _mesh;
        }
      }

      /**
       * \brief Returns the mesh of this node.
       * \returns
       * A pointer to the mesh contained in this node.
       */
      MeshType* get_mesh()
      {
        return _mesh;
      }

      /** \copydoc get_mesh() */
      const MeshType* get_mesh() const
      {
        return _mesh;
      }

      /**
       * \brief Adds a new submesh child node.
       *
       * \param[in] id
       * The id of the child node.
       *
       * \param[in] submesh_node
       * A pointer to the submesh node to be added.
       *
       * \param[in] chart
       * A pointer to the chart that the subnode is to be associated with. May be \c nullptr.
       *
       * \returns
       * \p submesh_node if the insertion was successful, otherwise \c nullptr.
       */
      SubMeshNodeType* add_submesh_node(
        Index id,
        SubMeshNodeType* submesh_node,
        const MeshChartType* chart = nullptr)
      {
        CONTEXT(name() + "::add_submesh_node()");
        if(submesh_node != nullptr)
        {
          if(_submesh_nodes.insert(std::make_pair(id,SubMeshNodeBin(submesh_node, chart))).second)
          {
            return submesh_node;
          }
        }
        return nullptr;
      }

      /**
       * \brief Searches for a submesh node.
       *
       * \param[in] id
       * The id of the node to be found.
       *
       * \returns
       * A pointer to the submesh node associated with \p id or \c nullptr if no such node was found.
       */
      SubMeshNodeType* find_submesh_node(Index id)
      {
        CONTEXT(name() + "::find_submesh_node()");
        SubMeshNodeIterator it(_submesh_nodes.find(id));
        return (it != _submesh_nodes.end()) ? it->second.node : nullptr;
      }

      /** \copydoc find_submesh_node() */
      const SubMeshNodeType* find_submesh_node(Index id) const
      {
        CONTEXT(name() + "::find_submesh_node() [const]");
        SubMeshNodeConstIterator it(_submesh_nodes.find(id));
        return (it != _submesh_nodes.end()) ? it->second.node : nullptr;
      }

      /**
       * \brief Searches for a submesh.
       *
       * \param[in] id
       * The id of the node associated with the submesh to be found.
       *
       * \returns
       * A pointer to the submesh associated with \p id or \c nullptr if no such node was found.
       */
      SubMeshType* find_submesh(Index id)
      {
        CONTEXT(name() + "::find_submesh()");
        SubMeshNodeType* node = find_submesh_node(id);
        return (node != nullptr) ? node->get_mesh() : nullptr;
      }

      /** \copydoc find_submesh() */
      const SubMeshType* find_submesh(Index id) const
      {
        CONTEXT(name() + "::find_submesh() [const]");
        const SubMeshNodeType* node = find_submesh_node(id);
        return (node != nullptr) ? node->get_mesh() : nullptr;
      }

      /**
       * \brief Searches for a submesh chart.
       *
       * \param[in] id
       * The id of the node associated with the chart to be found.
       *
       * \returns
       * A pointer to the chart associated with \p id of \c nullptr if no such node was found or if
       * the corresponding node did not have a chart.
       */
      MeshChartType* find_submesh_chart(Index id)
      {
        CONTEXT(name() + "::find_submesh_chart()");
        SubMeshNodeIterator it(_submesh_nodes.find(id));
        return (it != _submesh_nodes.end()) ? it->second.chart : nullptr;
      }

      /** \copydoc find_submesh_chart() */
      const MeshChartType* find_submesh_chart(Index id) const
      {
        CONTEXT(name() + "::find_submesh_chart() [const]");
        SubMeshNodeConstIterator it(_submesh_nodes.find(id));
        return (it != _submesh_nodes.end()) ? it->second.chart : nullptr;
      }

      /**
       * \brief Adapts this mesh node.
       *
       * This function loops over all submesh nodes and uses their associated charts (if given)
       * to adapt the mesh in this node.
       *
       * \param[in] recursive
       * If set to \c true, all submesh nodes are adapted prior to adapting this node.
       */
      void adapt(bool recursive = true)
      {
        CONTEXT(name() + "::adapt()");

        // loop over all submesh nodes
        SubMeshNodeIterator it(_submesh_nodes.begin());
        SubMeshNodeIterator jt(_submesh_nodes.end());
        for(; it != jt; ++it)
        {
          // adapt child node
          if(recursive)
          {
            it->second.node->adapt(true);
          }

          // adapt this node if a chart is given
          if(it->second.chart != nullptr)
          {
            it->second.chart->adapt(*_mesh, *(it->second.node->_mesh));
          }
        }
      }

      /**
       * \brief Adapts this mesh node.
       *
       * This function adapts this node by a specific chart whose id is given.
       *
       * \param[in] id
       * The id of the submesh node that is to be used for adaption.
       *
       * \param[in] recursive
       * If set to \c true, the submesh node associated with \p id will be adapted prior to adapting this node.
       *
       * \returns
       * \c true if this node was adapted successfully or \c false if no node is associated with \p id or if
       * the node did not contain any chart.
       */
      bool adapt_by_id(Index id, bool recursive = false)
      {
        CONTEXT(name() + "::adapt_by_id()");

        // try to find the corresponding submesh node
        SubMeshNodeIterator it(_submesh_nodes.find(id));
        if(it == _submesh_nodes.end())
          return false;

        // adapt child node
        if(recursive)
        {
          it->second.node->adapt(true);
        }

        // adapt this node
        if(it->second.chart != nullptr)
        {
          it->second.chart->adapt(*_mesh, *(it->second.node->_mesh));
          return true;
        }

        // no chart associated
        return false;
      }

      /**
       * \brief Returns the name of the class.
       * \returns
       * The name of the class as a String.
       */
      static String name()
      {
        return "MeshNode<...>";
      }

    protected:
      /**
       * \brief Refines all child nodes of this node.
       *
       * \param[in,out] refined_node
       * A reference to the node generated by refining this node.
       */
      void refine_children(MeshNode& refined_node) const
      {
        // refine submeshes
        refine_submeshes(refined_node);

        // refine subsets
        refine_subsets(refined_node);
      }

      /**
       * \brief Refines all child submesh nodes of this node.
       *
       * \param[in,out] refined_node
       * A reference to the node generated by refining this node.
       */
      void refine_submeshes(MeshNode& refined_node) const
      {
        SubMeshNodeConstIterator it(_submesh_nodes.begin());
        SubMeshNodeConstIterator jt(_submesh_nodes.end());
        for(; it != jt; ++it)
        {
          refined_node.add_submesh_node(it->first, it->second.node->refine(*_mesh), it->second.chart);
        }
      }

      /**
       * \brief Refines all child cell subset nodes of this node.
       *
       * \param[in,out] refined_node
       * A reference to the node generated by refining this node.
       */
      void refine_subsets(MeshNode& refined_node) const
      {
        typename BaseClass::CellSubSetNodeConstIterator it(this->_subset_nodes.begin());
        typename BaseClass::CellSubSetNodeConstIterator jt(this->_subset_nodes.end());
        for(; it != jt; ++it)
        {
          refined_node.add_subset_node(it->first, it->second->refine(*_mesh));
        }
      }
    }; // class MeshNode

    /* ***************************************************************************************** */

    /**
     * \brief Root mesh node class template
     *
     * This class template is used for the root node of a mesh tree.
     *
     * \author Peter Zajac
     */
    template<typename Policy_>
    class RootMeshNode
      : public MeshNode<Policy_, RootMeshNodePolicy<Policy_> >
    {
    public:
      /// base class typedef
      typedef MeshNode<Policy_, RootMeshNodePolicy<Policy_> > BaseClass;

      /// the mesh type of this node
      typedef typename BaseClass::MeshType MeshType;

    public:
      /**
       * \brief Constructor.
       *
       * \param[in] mesh
       * A pointer to the mesh for this node.
       */
      explicit RootMeshNode(MeshType* mesh) :
        BaseClass(mesh)
      {
      }

      /**
       * \brief Constructs a RootMeshNode from a streamed mesh
       *
       * \param[in] mesh_reader
       * MeshStreamer that contains the information from the streamed mesh.
       *
       */
      explicit RootMeshNode(MeshStreamer& mesh_reader) :
        BaseClass(nullptr)
      {
        // Construct a new Geometry::Mesh using the MeshStreamer and a MeshStreamerFactory
        MeshStreamerFactory<MeshType> my_factory(mesh_reader);
        this->_mesh = new MeshType(my_factory);

        // Generate a MeshStreamer::MeshNode, as this then contains the information about the SubMeshes and
        // CellSubSets present in the MeshStreamer
        MeshStreamer::MeshNode* root(mesh_reader.get_root_mesh_node());
        ASSERT_(root != nullptr);

        // Add SubMeshNodes and CellSubSetNodes to the new RootMeshNode by iterating over the MeshStreamer::MeshNode
        // Careful: MeshStreamer::SubMeshNodes and MeshStreamer::CellSubSetNodes use Strings as their names and
        // identifiers, whereas the RootMeshNode uses an Index for this
        Index i(0);
        for(auto& it:root->sub_mesh_map)
        {
          // Create a factory for the SubMesh
          MeshStreamerFactory<typename Policy_::SubMeshType> submesh_factory(mesh_reader, it.first);
          // Construct the SubMesh using that factory
          typename Policy_::SubMeshType* my_submesh(new typename Policy_::SubMeshType(submesh_factory));
          // Create the SubMeshNode using that SubMesh
          SubMeshNode<Policy_>* my_submesh_node(new SubMeshNode<Policy_>(my_submesh));
          // Add the new SubMeshNode to the RootMeshNode
          this->add_submesh_node(i++, my_submesh_node);
        }

        // Add CellSubsetNodes
        i = 0;
        for(auto& it:root->cell_set_map)
        {
          // Create a factory for the CellSubSet
          MeshStreamerFactory<typename Policy_::CellSubSetType> cell_subset_factory(mesh_reader, it.first);
          // Construct the CellSubSet using that factory
          typename Policy_::CellSubSetType* my_cell_subset
            (new typename Policy_::CellSubSetType(cell_subset_factory));
          // Create the CellSubSetNode using that CellSubSet
          CellSubSetNode<Policy_>* my_cell_subset_node(new CellSubSetNode<Policy_>(my_cell_subset));
          // Add the new CellSubSetNode to the RootMeshNode
          this->add_subset_node(i++, my_cell_subset_node);
        }

      }

      /// virtual destructor
      virtual ~RootMeshNode()
      {
      }

      /**
       * \brief Refines this node and its sub-tree.
       *
       * \returns
       * A pointer to a RootMeshNode containing the refined mesh tree.
       */
      RootMeshNode* refine() const
      {
        // create a refinery
        StandardRefinery<MeshType> refinery(*this->_mesh);

        // create a new root mesh node
        RootMeshNode* fine_node = new RootMeshNode(new MeshType(refinery));

        // refine our children
        this->refine_children(*fine_node);

        // okay
        return fine_node;
      }

      /**
       * \brief Returns the name of the class.
       * \returns
       * The name of the class as a String.
       */
      static String name()
      {
        return "RootMeshNode<...>";
      }
    }; // class RootMeshNode

    /* ***************************************************************************************** */

    /**
     * \brief Sub-Mesh node class template
     *
     * This class template is used for all mesh nodes of a mesh tree except for the root node.
     *
     * \author Peter Zajac
     */
    template<typename Policy_>
    class SubMeshNode
      : public MeshNode<Policy_, SubMeshNodePolicy<Policy_> >
    {
    public:
      /// base class typedef
      typedef MeshNode<Policy_, SubMeshNodePolicy<Policy_> > BaseClass;

      /// the mesh type of this node
      typedef typename BaseClass::MeshType MeshType;

    public:
      /**
       * \brief Constructor.
       *
       * \param[in] mesh
       * A pointer to the mesh for this node.
       */
      explicit SubMeshNode(MeshType* mesh) :
        BaseClass(mesh)
      {
      }

      /// virtual destructor
      virtual ~SubMeshNode()
      {
      }

      /**
       * \brief Refines this node and its sub-tree.
       *
       * \param[in] parent
       * A reference to the parent mesh of this node's submesh.
       *
       * \returns
       * A pointer to a SubMeshNode containing the refined mesh tree.
       */
      template<typename ParentType_>
      SubMeshNode* refine(const ParentType_& parent) const
      {
        // create a refinery
        StandardRefinery<MeshType, ParentType_> refinery(*this->_mesh, parent);

        // create a new sub-mesh node
        SubMeshNode* fine_node = new SubMeshNode(new MeshType(refinery));

        // refine our children
        this->refine_children(*fine_node);

        // okay
        return fine_node;
      }

      /**
       * \brief Returns the name of the class.
       * \returns
       * The name of the class as a String.
       */
      static String name()
      {
        return "SubMeshNode<...>";
      }
    }; // class SubMeshNode

  } // namespace Geometry
} // namespace FEAST

#endif // KERNEL_GEOMETRY_MESH_NODE_HPP

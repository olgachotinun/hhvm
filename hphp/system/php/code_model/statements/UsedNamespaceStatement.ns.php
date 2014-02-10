<?php
// Copyright 2004-present Facebook. All Rights Reserved.
// This file is @generated by
// fbcode/hphp/facebook/tools/code_model/Generate.sh

namespace HH\CodeModel {

/**
 *  use namespace [as alias];
 */
class UsedNamespaceStatement /*implements IUsedNamespaceStatement*/ {
  use Statement;
  use Node;

  private /*?string*/ $namespaceName;
  private /*?string*/ $aliasName;

  /**
   *  use namespace [as alias];
   */
  public function getNamespaceName() /*: string*/ {
    if ($this->namespaceName === null) {
      throw new \Exception("NamespaceName is not initialized");
    }
    return $this->namespaceName;
  }
  /**
   *  use namespace [as alias];
   */
  public function setNamespaceName(/*string*/ $value) /*: this*/ {
    $this->namespaceName = $value;
    return $this;
  }

  /**
   *  use namespace [as alias];
   */
  public function getAliasName() /*: string*/ {
    if ($this->aliasName === null) {
      throw new \Exception("AliasName is not initialized");
    }
    return $this->aliasName;
  }
  /**
   *  use namespace [as alias];
   */
  public function setAliasName(/*string*/ $value) /*: this*/ {
    $this->aliasName = $value;
    return $this;
  }

  /**
   * Returns $visitor->visitUsedNamespaceStatement($this) if
   * such a method exists on $visitor.
   * Otherwise returns $visitor->visitStatement($this) if
   * such a method exists on $visitor.
   * Otherwise returns $visitor->visitNode($this) if
   */
  public function accept(/*mixed*/ $visitor) /*: mixed*/ {
    if (method_exists($visitor, "visitUsedNamespaceStatement")) {
      // UNSAFE
      return $visitor->visitUsedNamespaceStatement($this);
    } else if (method_exists($visitor, "visitStatement")) {
      // UNSAFE
      return $visitor->visitStatement($this);
    } else {
      return $visitor->visitNode($this);
    }
  }

  /**
   * Yields a list of nodes that are children of this node.
   * A node has exactly one parent, so doing a depth
   * first traversal of a node graph using getChildren will
   * traverse a spanning tree.
   */
  public function getChildren() /*: Continuation<INode>*/ {
    // UNSAFE
    yield break;
  }
}
}
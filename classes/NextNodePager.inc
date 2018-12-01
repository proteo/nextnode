<?php

/**
 * @file
 * Custom NextNode pager class.
 */

/**
 * Query extender for pager queries.
 *
 * Extends the default pager class to ensure we can break "infinite" paged
 * queries without having an actual pager.
 */
class NextNodePager extends PagerDefault {

  /**
   * Current page defined by pager_default_initialize().
   *
   * @var int
   */
  protected $currentPage;

  /**
   * Current pager offset.
   *
   * @var int
   */
  protected $offset;

  /**
   * Total results number.
   *
   * @var int
   */
  protected $totalItems;

  /**
   * Override PagerDefault::execute().
   */
  public function execute() {
    // Add convenience tag to mark that this is an extended query. We have to
    // do this in the constructor to ensure that it is set before preExecute()
    // gets called.
    if (!$this->preExecute($this)) {
      return NULL;
    }

    // A NULL limit is the "kill switch" for pager queries.
    if (empty($this->limit)) {
      return NULL;
    }

    $this->ensureElement();
    $this->range($this->getOffset(), $this->limit);

    // Set the flag that will be picked by nextnode_is_last_page() when checking
    // if this is the last page of results available.
    define('NEXTNODE_QUERY_LIMIT_REACHED', $this->limitReached());

    // Now that we've added our pager-based range instructions, run the query
    // normally.
    return $this->query->execute();
  }

  /**
   * Calculate current page offset.
   */
  private function getOffset() {
    if (!isset($this->offset)) {
      $this->offset = $this->getCurrentPage() * $this->limit;
    }
    return $this->offset;
  }

  /**
   * Get current pager page.
   */
  private function getCurrentPage() {
    if (!isset($this->currentPage)) {
      $total_items = $this->getTotalItems();
      $this->currentPage = pager_default_initialize($total_items, $this->limit, $this->element);
    }
    return $this->currentPage;
  }

  /**
   * Calculate total results number.
   */
  private function getTotalItems() {
    if (!isset($this->totalItems)) {
      $this->totalItems = $this->getCountQuery()->execute()->fetchField();
    }
    return $this->totalItems;
  }

  /**
   * Checks if the current page reached the limit of available results.
   *
   * @see nextnode_is_last_page()
   */
  public function limitReached() {
    return (($this->getOffset() + 1) == $this->getTotalItems());
  }

}

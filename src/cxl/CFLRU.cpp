#include "CFLRU.h"

CFLRU::CFLRU(uint64_t cache_size) {
	if (cache_size >= 4096) {
		window = 4096;
	}
	else {
		window = cache_size / 2;
	}
}
CFLRU::~CFLRU() {
	while (head != NULL && head != tail) {
		LRUNode* n = head->right;
		delete head;
		head = n;
	}
	if (head != NULL) delete head;
	m.clear();
}

void CFLRU::add(uint64_t pn) {
	LRUNode* nnode{ new LRUNode };
	nnode->page_num = pn;

	if (head == NULL) {
		head = nnode;
		tail = nnode;
		m.emplace(pn, nnode);
		return;
	}

	nnode->right = head;
	head->left = nnode;
	head = nnode;

	m.emplace(pn, nnode);
}

uint64_t CFLRU::pop() {
	LRUNode* p{ tail }, * target{ NULL };
	uint64_t evict_addr{ 0 };

	for (auto i = 0; i < window; i++) {
		if (!p->dirty) {
			target = p;
			break;
		}
		p = p->left;
	}

	if (target != NULL && target != tail) {
		if (target == head) {
			target->right->left = NULL;
			head = target->right;
			evict_addr = target->page_num;
			m.erase(m.find(target->page_num));
			delete target;
			return evict_addr;
		}
		target->left->right = target->right;
		target->right->left = target->left;
		evict_addr = target->page_num;
		m.erase(m.find(target->page_num));
		delete target;
		return evict_addr;
	}

	target = tail;
	tail = target->left;
	tail->right = NULL;

	evict_addr = target->page_num;
	m.erase(m.find(target->page_num));
	delete target;

	return evict_addr;
}

void CFLRU::modify(uint64_t pn, bool dirty) {
	LRUNode* target{ m.find(pn)->second };

	if (target != head) {
		if (target == tail) {
			if (!target->dirty) target->dirty = dirty;
			tail = target->left;
			tail->right = NULL;

			head->left = target;
			target->right = head;
			target->left = NULL;
			head = target;
			return;
		}

		head->left = target;
		target->left->right = target->right;
		target->right->left = target->left;
		if (!target->dirty) target->dirty = dirty;
		target->right = head;
		target->left = NULL;
		head = target;
		return;
	}

	if (!target->dirty) target->dirty = dirty;
	return;
}

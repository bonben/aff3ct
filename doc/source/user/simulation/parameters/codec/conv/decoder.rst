.. _dec-conv-decoder-parameters:

|CONV| Decoder parameters
-------------------------

.. _dec-conv-dec-type:

``--dec-type, -D``
""""""""""""""""""

   :Type: text
   :Allowed values: ``VITERBI`` ``PLVA`` ``CHASE`` ``ML``
   :Default: ``VITERBI``
   :Examples: ``--dec-type VITERBI``

|factory::Decoder::p+type,D|

Description of the allowed values:

+--------------+------------------------------------------------------------------+
| Value        | Description                                                      |
+==============+==================================================================+
| ``VITERBI``  | Select the Viterbi algorithm from :cite:`Viterbi1967`.           |
+--------------+------------------------------------------------------------------+
| ``PLVA``     | Select the |PLVA| algorithm from :cite:`Seshadri1994`.           |
+--------------+------------------------------------------------------------------+
| ``CHASE``    | See the common :ref:`dec-common-dec-type` parameter.             |
+--------------+------------------------------------------------------------------+
| ``ML``       | See the common :ref:`dec-common-dec-type` parameter.             |
+--------------+------------------------------------------------------------------+

.. note:: Only rate-1/2 feedforward convolutional codes are supported. The
   |BCJR| algorithm is not available for this codec; for a soft-output decoder
   use the :ref:`Codec RSC <codec-rsc>` instead.

.. _dec-conv-dec-implem:

``--dec-implem``
""""""""""""""""

   :Type: text
   :Allowed values: ``STD``
   :Default: ``STD``
   :Examples: ``--dec-implem STD``

|factory::Decoder::p+implem|

.. _dec-conv-dec-lists:

``--dec-lists, -L``
"""""""""""""""""""

   :Type: integer
   :Default: ``8``
   :Examples: ``--dec-lists 32``

|factory::Decoder_conv::p+lists,L|

References
""""""""""

.. bibliography:: references.bib
   :labelprefix: Conv-

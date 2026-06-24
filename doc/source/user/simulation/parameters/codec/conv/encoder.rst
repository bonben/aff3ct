.. _enc-conv-encoder-parameters:

|CONV| Encoder parameters
-------------------------

.. _enc-conv-enc-info-bits:

``--enc-info-bits, -K`` |image_required_argument|
"""""""""""""""""""""""""""""""""""""""""""""""""

   :Type: integer
   :Examples: ``--enc-info-bits 64``

|factory::Encoder::p+info-bits,K|

The codeword size :math:`N` is automatically deduced:
:math:`N = 2 \times (K + \log_2(ts))` where :math:`ts` is the trellis size.

.. _enc-conv-enc-type:

``--enc-type``
""""""""""""""

   :Type: text
   :Allowed values: ``CONV`` ``AZCW`` ``COSET`` ``USER``
   :Default: ``CONV``
   :Examples: ``--enc-type AZCW``

|factory::Encoder::p+type|

Description of the allowed values:

+-----------+------------------------+
| Value     | Description            |
+===========+========================+
| ``CONV``  | |enc-type_descr_conv|  |
+-----------+------------------------+
| ``AZCW``  | |enc-type_descr_azcw|  |
+-----------+------------------------+
| ``COSET`` | |enc-type_descr_coset| |
+-----------+------------------------+
| ``USER``  | |enc-type_descr_user|  |
+-----------+------------------------+

.. |enc-type_descr_conv| replace:: Select the standard feedforward
   convolutional encoder.
.. |enc-type_descr_azcw| replace:: See the common :ref:`enc-common-enc-type`
   parameter.
.. |enc-type_descr_coset| replace:: See the common :ref:`enc-common-enc-type`
   parameter.
.. |enc-type_descr_user| replace:: See the common :ref:`enc-common-enc-type`
   parameter.

Unlike the :ref:`Codec RSC <codec-rsc>`, the convolutional encoder is
**feedforward** (non-recursive and non-systematic). The encoded output is
interleaved: for :math:`K` information bits and :math:`n_{\mathit{ff}} =
\log_2(ts)` memory elements, the encoder outputs
:math:`X_0^{(0)}, X_0^{(1)}, X_1^{(0)}, X_1^{(1)}, [...], X_{K-1}^{(0)},
X_{K-1}^{(1)}, X_{0}^{(0)^t}, X_{0}^{(1)^t}, [...],
X_{n_{\mathit{ff}}-1}^{(0)^t}, X_{n_{\mathit{ff}}-1}^{(1)^t}`, where
:math:`X^{(0)}` and :math:`X^{(1)}` are respectively the outputs of the first
and second generator polynomial, and :math:`t` the *tail bits* used to drive
the encoder back to the zero state.

.. _enc-conv-enc-poly:

``--enc-poly``
""""""""""""""

   :Type: text
   :Default: ``"{0171,0133}"``
   :Examples: ``--enc-poly "{023,033}"``

|factory::Encoder_conv::p+poly|

The default ``{0171,0133}`` corresponds to the industry-standard constraint
length 7, rate 1/2 NASA code, also used as the inner code of the Galileo E1B
signal.
